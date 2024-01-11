#pragma once
#include <vector>
#include <d3d11.h>
#include "types.h"
#include "GeometryBuilder.h"

class ParticleSystem
{
	std::vector<vec3_t> positions_;
	std::vector<vec3_t> velocities_;
	std::vector<float> lifetimes_;
	std::vector<float> rotations_2d_;
	std::vector<float> rotations_2d_over_time_;

	float next_spawn_timer_;

	public:
	ID3D11Buffer* vbuffer_;
	ID3D11Buffer* ibuffer_;
	GeometryBuilder builder_;
	int max_particles_;
	float spawn_rate_;
	vec3_t pos_;
	vec3_t target_velocity_;
	vec3_t spawn_volume_size_;
	float lifetime_;

	ParticleSystem(ID3D11Device* device, int max_particles);
	void Spawn();
	void UpdateAndRender(ID3D11DeviceContext* context, float delta_time, vec3_t camera_forward);
};


