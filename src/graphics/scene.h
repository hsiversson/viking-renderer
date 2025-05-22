#pragma once

namespace vkr::Graphics
{
	class Scene
	{
	public:
		Scene();
		~Scene();

		// These should be thread-safe
		// Create/Destroy view?
		// Add/Remove model/object?
		// Add/Remove light
		// Add/Remove this...
		// Add/Remove that...

		// Run per frame updates to the scene and its objects & views
		// Update()

		// Prepare render data for rendering for each view. 
		// I.e extract renderable information and store in list to be picked up by render tasks later
		// Prepare()

		// Launch actual gpu recording rendering tasks for each view
		// Render()

	private:
		// For now only a simple list of scene objects, 
		// but later maybe a spatial partitioning structure of scene objects?
		// Quadtree, Octree, Grid?
	};
}