#include <iostream>
#include <filesystem>
#include <fstream>
#include <regex>
#include <string>
#include <sstream>
#include <future>
#include <mutex>
#include <vector>
#include <algorithm>

const std::regex& struct_regex() 
{
    static const std::regex r(R"((?:struct|class)\s+([A-Za-z_]\w*)\s*\{)", std::regex::ECMAScript);
    return r;
}

const std::regex& property_regex() 
{
    static const std::regex r(R"(PROPERTY\(([^)]*)\)\s+([A-Za-z_][A-Za-z0-9_:<>]*)\s+([A-Za-z_][A-Za-z0-9_]*)\s*(?:=[^;]*)?;)", std::regex::ECMAScript);
    return r;
}

struct Property 
{
    std::string type;
    std::string name;
    std::string presentableName;
};

struct StructInfo
{
    std::string name;
    std::string namespaceName;
    std::vector<Property> props;
};

struct HeaderInfo
{
    std::filesystem::path path;
    std::vector<StructInfo> structs;
};

// Global reflection list (thread-safe)
std::mutex g_ListMutex;
std::vector<HeaderInfo> g_AllHeaders;

bool ProcessHeader(const std::filesystem::path& sourcePath, const std::filesystem::path& outputPath)
{
    std::ifstream file(sourcePath, std::ios::binary);
    if (!file.is_open())
        return false;

    std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    // Strip UTF-8 BOM if present
    if (text.size() >= 3 && (unsigned char)text[0] == 0xEF && (unsigned char)text[1] == 0xBB && (unsigned char)text[2] == 0xBF)
        text.erase(0, 3);

    bool anyGenerated = false;

    std::filesystem::path outPath = outputPath;
    outPath.replace_extension(".generated.h");

    HeaderInfo header;
    header.path = outPath;

    std::ostringstream out;
    out << "// Auto-generated type reflection information.\n";
    out << "#pragma once\n";
    out << "#include \"core/reflection.h\"\n\n";

    std::string headerIncludePath = sourcePath.string();
    std::replace(headerIncludePath.begin(), headerIncludePath.end(), '\\', '/');
    out << "#include \"" << headerIncludePath << "\"\n\n";
    out << "namespace vkr\n{\n";

    // Namespace stack
    std::vector<std::string> namespaceStack;

    size_t i = 0;
    while (i < text.size())
    {
        // Skip whitespace
        while (i < text.size() && isspace(text[i])) i++;
        if (i >= text.size()) break;

        // --- namespace handling ---
        if (text.compare(i, 9, "namespace") == 0 && isspace(text[i + 9]))
        {
            i += 9;
            while (i < text.size() && isspace(text[i])) i++;

            // find the full namespace name up to '{'
            size_t bracePos = text.find('{', i);
            if (bracePos == std::string::npos) break;

            std::string nsFull = text.substr(i, bracePos - i);
            // trim trailing spaces
            size_t endTrim = nsFull.find_last_not_of(" \t\r\n");
            if (endTrim != std::string::npos) nsFull = nsFull.substr(0, endTrim + 1);

            // split by "::" and push each to the stack
            size_t pos = 0;
            while (pos < nsFull.size())
            {
                size_t sep = nsFull.find("::", pos);
                if (sep == std::string::npos) sep = nsFull.size();
                std::string nsPart = nsFull.substr(pos, sep - pos);
                if (!nsPart.empty())
                    namespaceStack.push_back(nsPart);
                pos = sep + 2;
            }

            i = bracePos + 1; // skip '{'
            continue;
        }

        // --- struct/class handling ---
        if (text.compare(i, 6, "struct") == 0 || text.compare(i, 5, "class") == 0)
        {
            size_t kwLen = (text[i] == 's') ? 6 : 5;
            i += kwLen;
            while (i < text.size() && isspace(text[i])) i++;

            size_t nameEnd = i;
            while (nameEnd < text.size() && (isalnum(text[nameEnd]) || text[nameEnd]=='_')) nameEnd++;
            if (nameEnd == i) continue; // no name found

            std::string structName = text.substr(i, nameEnd - i);
            i = nameEnd;

            StructInfo s;
            s.name = structName;
            s.namespaceName.clear();
            for (const auto& ns : namespaceStack) s.namespaceName += ns + "::";

            // Find opening brace
            while (i < text.size() && text[i] != '{') i++;
            if (i >= text.size()) break;

            int braceDepth = 1;
            size_t j = i + 1;
            for (; j < text.size() && braceDepth > 0; ++j)
            {
                if (text[j] == '{') braceDepth++;
                else if (text[j] == '}') braceDepth--;
            }

            if (braceDepth != 0) { i = j; continue; }

            size_t braceClose = j;
            std::string body = text.substr(i, braceClose - i);

            // Extract properties
            std::sregex_iterator propBegin(body.begin(), body.end(), property_regex());
            std::sregex_iterator propEnd;
            for (auto pit = propBegin; pit != propEnd; ++pit)
            {
                Property p;
                p.type = (*pit)[2];
                p.name = (*pit)[3];
                p.presentableName = p.name.starts_with("m_") ? p.name.substr(2) : p.name;
                s.props.push_back(p);
            }

            if (!s.props.empty())
            {
                out << "    template<>\n";
                out << "    struct Reflection<" << s.namespaceName << s.name << "> \n";
                out << "    {\n";
                out << "        static constexpr bool m_IsReflected = true;\n";
                out << "        static constexpr std::string_view m_TypeName = \"" << s.name << "\";\n";
                out << "        static constexpr auto m_Properties = std::make_tuple(\n";

                for (size_t k = 0; k < s.props.size(); ++k)
                {
                    const Property& p = s.props[k];
                    out << "            ReflectedProperty<" << s.namespaceName << s.name << ", " << p.type
                        << ">{ \"" << p.presentableName << "\", &" << s.namespaceName << s.name << "::" << p.name << " }";
                    if (k + 1 < s.props.size()) out << ",";
                    out << "\n";
                }

                out << "        );\n";
                out << "    };\n";

                header.structs.push_back(s);
                anyGenerated = true;
            }

            i = braceClose;
            continue;
        }

        // --- handle closing brace (namespace pop) ---
        if (text[i] == '}')
        {
            if (!namespaceStack.empty())
                namespaceStack.pop_back();
        }

        i++;
    }

    out << "}\n";

    if (anyGenerated)
    {
        std::unique_lock lock(g_ListMutex);
        g_AllHeaders.push_back(header);

        std::ofstream outFile(outPath, std::ios::binary);
        outFile << out.str();
        std::cout << "Generated: " << outPath << "\n";
        return true;
    }

    return false;
}

int main(int argc, char** argv)
{
    std::filesystem::path repositoryRoot = argv[0];
    repositoryRoot = std::filesystem::canonical(repositoryRoot.parent_path() / ".." / "..");
    std::filesystem::path outputDir = repositoryRoot / "build" / "generated";

    if (argc < 2)
    {
        std::cerr << "Usage: VikingHeaderTool.exe <directory>\n";
        return 1;
    }

    std::filesystem::path root = argv[1];
    if (!std::filesystem::exists(root))
    {
        std::cerr << "Error: Path does not exist: " << root << "\n";
        return 1;
    }

    std::vector<std::future<bool>> tasks;

    for (auto& entry : std::filesystem::recursive_directory_iterator(root))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".h")
        {
            std::filesystem::path sourcePath = entry.path();
            std::filesystem::path relative = std::filesystem::relative(sourcePath, root);
            std::filesystem::path outputPath = outputDir / relative;

            std::filesystem::create_directories(outputPath.parent_path());
            tasks.push_back(std::async(&ProcessHeader, sourcePath, outputPath));
        }
    }

    for (std::future<bool>& task : tasks)
        task.wait();

    // ---- Generate monolithic reflection registry ----
    {
        std::filesystem::path registerAllPath = outputDir / "register_reflections.generated.h";
        std::ofstream registerAllHeader(registerAllPath);

        registerAllHeader << "// Auto-generated reflection register.\n";
        registerAllHeader << "namespace vkr::RegisterReflections\n";
        registerAllHeader << "{\n";
        registerAllHeader << "   void RegisterAll();\n";
        registerAllHeader << "}\n";
        std::cout << "Generated register all header: " << registerAllPath << "\n";
    }
    {
        std::filesystem::path registerAllPath = outputDir / "register_reflections.generated.cpp";
        std::ofstream registerAllSrc(registerAllPath);

        registerAllSrc << "// Auto-generated reflection register.\n";
        registerAllSrc << "#include \"register_reflections.generated.h\"\n";
        registerAllSrc << "#include \"core/reflection.h\"\n";

        // Include all reflection headers
        for (const HeaderInfo& header : g_AllHeaders)
        {
            std::string includePath = std::filesystem::relative(header.path, outputDir).string();
            std::replace(includePath.begin(), includePath.end(), '\\', '/');
            registerAllSrc << "#include \"" << includePath << "\"\n";
        }

        registerAllSrc << "\n";
        registerAllSrc << "namespace vkr::RegisterReflections\n";
        registerAllSrc << "{\n";
        registerAllSrc << "   void RegisterAll()\n";
        registerAllSrc << "   {\n";
        for (const HeaderInfo& header : g_AllHeaders)
        {
            for (const StructInfo& s : header.structs)
            {
                registerAllSrc << "       REGISTER_TYPE_REFLECTION(" << s.namespaceName << s.name << "," << s.name << ");\n";
            }
        }
        registerAllSrc << "   }\n";
        registerAllSrc << "}\n";

        std::cout << "Generated register all src: " << registerAllPath << "\n";
    }
    std::cout << "--- FINISHED ---\n";
    return 0;
}
