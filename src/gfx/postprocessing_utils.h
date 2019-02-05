#pragma once

#include <core/gl.h>
#include <core/types.h>
#include <core/vector_types.h>
#include <gfx/view_param.h>

namespace postprocessing {

void initialize(int width, int height);
void shutdown();

typedef int Tonemapping;
enum Tonemapping_ { Tonemapping_Passthrough, Tonemapping_ExposureGamma, Tonemapping_Filmic };

struct PostProcessingDesc {
	struct {
		bool enabled = true;
		float clip_point = 1.0f;
	} bloom;

	struct {
		bool enabled = true;
		Tonemapping mode = Tonemapping_Filmic;
		float exposure = 1.0f;
		float gamma = 2.2f;
	} tonemapping;

	struct {
		bool enabled = true;
		float radius = 6.0f;
		float intensity = 3.0f;
		float bias = 0.1f;
	} ambient_occlusion;

	struct {
		bool enabled = true;
		float focus_depth = 0.5f;
		float focus_scale = 10.f;
	} depth_of_field;

	struct {
		bool enabled = true;
		float feedback_min = 0.88f;
		float feedback_max = 0.88f;
		struct {
			bool enabled = true;
			float motion_scale = 1.f;
		} motion_blur;
	} temporal_reprojection;
};

void apply_postprocessing(const PostProcessingDesc& desc, const ViewParam& view_param, GLuint depth_tex, GLuint color_tex, GLuint normal_tex, GLuint velocity_tex);

//void compute_linear_depth(GLuint depth_tex, float near_plane, float far_plane, bool orthographic = false);
void blit_velocity(const ViewParam& view_param);

void shade_deferred(GLuint depth_tex, GLuint color_tex, GLuint normal_tex, const mat4& inv_proj_matrix);
void highlight_selection(GLuint atom_idx_tex, GLuint selection_buffer);
void apply_ssao(GLuint depth_tex, GLuint normal_tex, const mat4& proj_mat, float intensity = 1.5f, float radius = 3.f, float bias = 0.1f);
void apply_dof(GLuint depth_tex, GLuint color_tex, const mat4& proj_mat, float focus_point, float focus_scale);
void apply_tonemapping(GLuint color_tex, Tonemapping tonemapping, float exposure = 1.0f, float gamma = 2.2f);
void apply_temporal_aa(GLuint depth_tex, GLuint color_tex, GLuint velocity_tex, const ViewParam& param);
void blit_texture(GLuint tex);

}  // namespace postprocessing
