#include "ParticleSystem.h"

static uint32_t random_next_seed(uint32_t* state)
{
    //uint32_t t = *state;
    *state ^= *state << 13;
    *state ^= *state >> 17;
    *state ^= *state << 5;
    //return t;
    return *state;
}

// Range is min inclusive and max exclusive. Eveness of distribution untested, TODO. Formula taken from here: https://www.geeksforgeeks.org/generating-random-number-range-c/
uint32_t rdx_random_in_range(uint32_t* seed, int32_t min, int32_t max)
{
    //std::assert(min <= max);
    uint32_t range = (uint32_t)(max - min);
    uint32_t rand = random_next_seed(seed);
    return (rand % range) + min;
}

float rdx_rand_range_f(uint32_t* seed, float min, float max)
{
    random_next_seed(seed);
    double r = ((double)*seed) / UINT32_MAX;

    return r * (max - min) + min;
}

ParticleSystem::ParticleSystem(ID3D11Device* device, int max_particles)
{
    max_particles_ = max_particles;
    spawn_rate_ = 0.1f;
    lifetime_ = 5.0f;

	positions_.reserve(max_particles);
	velocities_.reserve(max_particles);
    lifetimes_.reserve(max_particles);
    rotations_2d_.reserve(max_particles);

    {
        D3D11_BUFFER_DESC desc = {
            .ByteWidth = static_cast<UINT>(max_particles * 4 * sizeof(builder_.vert[0])),
            .Usage = D3D11_USAGE_DYNAMIC,
            .BindFlags = D3D11_BIND_VERTEX_BUFFER,
            .CPUAccessFlags = D3D10_CPU_ACCESS_WRITE,
        };

        device->CreateBuffer(&desc, nullptr, &vbuffer_);
    }

    {
        D3D11_BUFFER_DESC desc = {
            .ByteWidth = static_cast<UINT>(max_particles * 6 * sizeof(builder_.ind[0])),
            .Usage = D3D11_USAGE_DYNAMIC,
            .BindFlags = D3D11_BIND_INDEX_BUFFER,
            .CPUAccessFlags = D3D10_CPU_ACCESS_WRITE,
        };

        D3D11_SUBRESOURCE_DATA initial = { .pSysMem = builder_.ind.data() };
        device->CreateBuffer(&desc, nullptr, &ibuffer_);
    }
}

void ParticleSystem::Spawn()
{
    if (positions_.size() >= max_particles_ - 1) {
        printf("Can't spawn new particle, max_particles count reached!\n");
        return;
    }

    static uint32_t seed = 42;

    vec3_t position = pos_;
    position.x += rdx_rand_range_f(&seed, -spawn_volume_size_.x, spawn_volume_size_.x);
    position.y += rdx_rand_range_f(&seed, -spawn_volume_size_.y, spawn_volume_size_.y);
    position.z += rdx_rand_range_f(&seed, -spawn_volume_size_.z, spawn_volume_size_.z);
	positions_.push_back(position);

    vec3_t vel = vec3(rdx_rand_range_f(&seed, -0.5f, 0.5f), rdx_rand_range_f(&seed, -0.5f, 0.5f), rdx_rand_range_f(&seed, -0.5f, 0.5f));
    velocities_.push_back(vel * 20);

    lifetimes_.push_back(lifetime_);

    rotations_2d_.push_back(rdx_rand_range_f(&seed, 0, 3.14f * 2));

    rotations_2d_over_time_.push_back(rdx_rand_range_f(&seed, -10, 10));
}


void ParticleSystem::UpdateAndRender(ID3D11DeviceContext* context, float delta_time, vec3_t camera_forward)
{
    // Spawn new particles
    if (next_spawn_timer_ <= 0.0f) {
        Spawn();
        next_spawn_timer_ = spawn_rate_;
    }
    next_spawn_timer_ -= delta_time;

    // Update the particles simulation
    for (int i = 0; i < positions_.size(); i++) {
        velocities_[i] = velocities_[i] * 0.9f + target_velocity_ * 0.1f;
        positions_[i] = positions_[i] + velocities_[i] * delta_time;
    }

    // Delete old particles
    for (int i = 0; i < positions_.size(); i++) {
        lifetimes_[i] -= delta_time;
        if (lifetimes_[i] <= 0) {
            // delete the particle by "swap and pop" 
            lifetimes_[i] = lifetimes_[lifetimes_.size()-1];
            lifetimes_.pop_back();
            positions_[i] = positions_[positions_.size() - 1];
            positions_.pop_back();
            velocities_[i] = velocities_[velocities_.size() - 1];
            velocities_.pop_back();
            rotations_2d_[i] = rotations_2d_[rotations_2d_.size() - 1];
            rotations_2d_.pop_back();
            rotations_2d_over_time_[i] = rotations_2d_over_time_[rotations_2d_over_time_.size() - 1];
            rotations_2d_over_time_.pop_back();
        }
    }

    // Teleport particles that are outside of the allowed zone
    for (int i = 0; i < positions_.size(); i++) {
        if (positions_[i].x > pos_.x + spawn_volume_size_.x) {
            positions_[i].x -= spawn_volume_size_.x * 2;
        }
        if (positions_[i].x < pos_.x - spawn_volume_size_.x) {
            positions_[i].x += spawn_volume_size_.x * 2;
        }
        if (positions_[i].z > pos_.z + spawn_volume_size_.z) {
            positions_[i].z -= spawn_volume_size_.z * 2;
        }
        if (positions_[i].z < pos_.z -spawn_volume_size_.z) {
            positions_[i].z += spawn_volume_size_.z * 2;
        }
    }

    // Generate the mesh
    builder_.vert.clear();
    builder_.ind.clear();
    vec3_t up = vec3(0, 1, 0);
    vec3_t forward = camera_forward * -1;
    vec3_t right = vec3_dot(up, forward);

	for (int i = 0; i < positions_.size(); i++) {
        float t = lifetimes_[i] / lifetime_;
        float particle_size = 1.0f - ((t*2 - 1.f) * (t*2 - 1.f));
        particle_size *= .3f;
		vec3_t center = positions_[i];
        float r = rotations_2d_[i];
        r += t * rotations_2d_over_time_[i];
        vec3_t local_x = (right * cos(r) + up * -sin(r)) * particle_size;
        vec3_t local_y = (right * sin(r) + up * cos(r)) * particle_size;

        vec3_t a = center - local_x - local_y;
        vec3_t b = center - local_x + local_y;
        vec3_t c = center + local_x + local_y;
        vec3_t d = center + local_x - local_y;
        float col = 1;
        float opacity = t;
		builder_.PushQuad(a,b,c,d,vec3(1,1,1),t);
	}

    // Update vertex buffer
    {
        D3D11_MAPPED_SUBRESOURCE mapped_resource = {};
        context->Map(vbuffer_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource);
        memcpy(mapped_resource.pData, builder_.vert.data(), builder_.vert.size() * sizeof(builder_.vert[0]));
        context->Unmap(vbuffer_, 0);
    }

    // Update index buffer
    {
        D3D11_MAPPED_SUBRESOURCE mapped_resource = {};
        context->Map(ibuffer_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource);
        memcpy(mapped_resource.pData, builder_.ind.data(), builder_.ind.size() * sizeof(builder_.ind[0]));
        context->Unmap(ibuffer_, 0);
    }

    UINT stride = sizeof(struct Vertex);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, &vbuffer_, &stride, &offset);
    context->IASetIndexBuffer(ibuffer_, DXGI_FORMAT_R32_UINT, 0);

    // draw
    context->DrawIndexed(builder_.ind.size(), 0, 0);
}
