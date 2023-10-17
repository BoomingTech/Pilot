﻿#pragma once
#include <vector>
#include "database.h"
#include "anim_instance.h"
#include "runtime/core/math/vector3.h"
#include "neural_network.h"

namespace Piccolo{
	enum Bones
	{
		Bone_Entity = 0,
		Bone_Hips = 1,
		Bone_LeftUpLeg = 2,
		Bone_LeftLeg = 3,
		Bone_LeftFoot = 4,
		Bone_LeftToe = 5,
		Bone_RightUpLeg = 6,
		Bone_RightLeg = 7,
		Bone_RightFoot = 8,
		Bone_RightToe = 9,
		Bone_Spine = 10,
		Bone_Spine1 = 11,
		Bone_Spine2 = 12,
		Bone_Neck = 13,
		Bone_Head = 14,
		Bone_LeftShoulder = 15,
		Bone_LeftArm = 16,
		Bone_LeftForeArm = 17,
		Bone_LeftHand = 18,
		Bone_RightShoulder = 19,
		Bone_RightArm = 20,
		Bone_RightForeArm = 21,
		Bone_RightHand = 22,
		Bone_Count,
	};

	struct MapPair
    {
        MapPair(std::string src, std::string dest) : src_name(std::move(src)), dest_name(std::move(dest)) {}
        std::string src_name;
        std::string dest_name;
    };

    struct MMPair
    {
        MMPair(Bones src, std::string dst) : src_idx(src), dest_name(std::move(dst)) {}
        Bones       src_idx;
        std::string dest_name;
    };

    
	struct SAnimationFrame
    {
        std::vector<std::string>         m_bone_names;
        std::vector<Piccolo::Vector3>    m_position;
        std::vector<Piccolo::Quaternion> m_rotation;
        std::vector<Piccolo::Vector3>    m_scaling;
        std::vector<uint32_t>            m_parents;
    };

	REFLECTION_TYPE(CAnimInstanceMotionMatching)

    CLASS(CAnimInstanceMotionMatching : public CAnimInstanceBase, WhiteListFields)
    {
        REFLECTION_BODY(CAnimInstanceMotionMatching)
	
	public:
		CAnimInstanceMotionMatching();
		CAnimInstanceMotionMatching(AnimationComponentRes* res);
		~CAnimInstanceMotionMatching() override = default;
		void TickAnimation(float delta_time) override;
        Skeleton* GetSkeleton() override { return &m_skeleton; }
	private:
        Skeleton m_skeleton;
        // 选小的当做映射表 找个地方存一下这个rig映射
        std::vector<MapPair> s_map_pairs;
        std::vector<MMPair>  s_mm_pairs;
		Vector3 m_root_motion;
        Quaternion m_root_rotation;
        // debug info
        std::vector<Piccolo::Vector3> m_debug_position;
        std::vector<Piccolo::Vector3> m_debug_direction;

        std::vector<Piccolo::Vector3> m_matched_position;
        std::vector<Piccolo::Vector3> m_matched_direction;
        SAnimationFrame m_result_frame;

		float m_crrent_time{0.0f};
		CDataBase m_data_base;

		float m_feature_weight_foot_position{0.75f};
		float m_feature_weight_foot_velocity{1.0f};
		float m_feature_weight_hip_velocity{1.0f};
		float m_feature_weight_trajectory_positions{1.0f};
		float m_feature_weight_trajectory_directions{1.5f};

		// Pose & Inertializer Data
		int m_frame_index{0};
		float m_inertialize_blending_halflife{0.1f};
		Array1D<Piccolo::Vector3> m_curr_bone_positions;
		Array1D<Piccolo::Vector3> m_curr_bone_velocities;
		Array1D<Piccolo::Quaternion> m_curr_bone_rotations;
		Array1D<Piccolo::Vector3> m_curr_bone_angular_velocities;
		Array1D<bool> m_curr_bone_contacts;

		Array1D<Piccolo::Vector3> m_trns_bone_positions;
		Array1D<Piccolo::Vector3> m_trns_bone_velocities;
		Array1D<Piccolo::Quaternion> m_trns_bone_rotations;
		Array1D<Piccolo::Vector3> m_trns_bone_angular_velocities;
		Array1D<bool> m_trns_bone_contacts;

		Array1D<Piccolo::Vector3> m_bone_positions;
		Array1D<Piccolo::Vector3> m_bone_velocities;
		Array1D<Piccolo::Quaternion> m_bone_rotations;
		Array1D<Piccolo::Vector3> m_bone_angular_velocities;

		Array1D<Piccolo::Vector3> m_bone_offset_positions;
		Array1D<Piccolo::Vector3> m_bone_offset_velocities;
		Array1D<Piccolo::Quaternion> m_bone_offset_rotations;
		Array1D<Piccolo::Vector3> m_bone_offset_angular_velocities;


		Piccolo::Vector3 m_transition_src_position;
		Piccolo::Quaternion m_transition_src_rotation;
		Piccolo::Vector3 m_transition_dst_position;
		Piccolo::Quaternion m_transition_dst_rotation;

		// Trajectory & Gameplay Data
		float m_search_time{0.1f};
		float m_search_timer;
		float m_force_search_timer;

		Piccolo::Vector3 m_desired_velocity;
		Piccolo::Vector3 m_desired_velocity_change_curr;
		Piccolo::Vector3 m_desired_velocity_change_prev;
		float m_desired_velocity_change_threshold{50.0};

		Piccolo::Quaternion m_desired_rotation;
		Piccolo::Vector3 m_desired_rotation_change_curr;
		Piccolo::Vector3 m_desired_rotation_change_prev;
		float m_desired_rotation_change_threshold{50.0};

		float m_desired_gait{0.0f};
		float m_desired_gait_velocity{0.0f};

		Piccolo::Vector3 m_simulation_position;
		Piccolo::Vector3 m_simulation_velocity;
		Piccolo::Vector3 m_simulation_acceleration;
		Piccolo::Quaternion m_simulation_rotation;
		Piccolo::Vector3 m_simulation_angular_velocity;

		float m_simulation_velocity_halflife{0.27f};
		float m_simulation_rotation_halflife{0.27f};

		// All speeds in m/s
		float m_simulation_run_fwrd_speed{4.0f};
		float m_simulation_run_side_speed{3.0f};
		float m_simulation_run_back_speed{2.5f};

		float m_simulation_walk_fwrd_speed{1.75f};
		float m_simulation_walk_side_speed{1.5f};
		float m_simulation_walk_back_speed{1.25f};

		Array1D<Piccolo::Vector3> m_trajectory_desired_velocities;
		Array1D<Piccolo::Quaternion> m_trajectory_desired_rotations;
		Array1D<Piccolo::Vector3> m_trajectory_positions;
		Array1D<Piccolo::Vector3> m_trajectory_velocities;
		Array1D<Piccolo::Vector3> m_trajectory_accelerations;
		Array1D<Piccolo::Quaternion> m_trajectory_rotations;
		Array1D<Piccolo::Vector3> m_trajectory_angular_velocities;

		//lmm
        CNeuralNetwork m_decompressor;
        Array2D<float> m_latent;
        bool           m_lmm_enable {true};


		void BuildMatchingFeature(CDataBase& db,
		                          const float feature_weight_foot_position,
		                          const float feature_weight_foot_velocity,
		                          const float feature_weight_hip_velocity,
		                          const float feature_weight_trajectory_positions,
		                          const float feature_weight_trajectory_directions);
		void ComputeBonePositionFeature(CDataBase& db, int& offset, int bone, float weight = 1.0f);
		void ComputeBoneVelocityFeature(CDataBase& db, int& offset, int bone, float weight = 1.0f);
		void ComputeTrajectoryPositionFeature(CDataBase& db, int& offset, float weight = 1.0f);
		void ComputeTrajectoryDirectionFeature(CDataBase& db, int& offset, float weight = 1.0f);

		void DataBaseBuildBounds(CDataBase& db);
		void RecursiveForwardKinematics(Piccolo::Vector3& bone_position,
		                                Piccolo::Quaternion& bone_rotation,
		                                const Slice1D<Piccolo::Vector3> bone_positions,
		                                const Slice1D<Piccolo::Quaternion> bone_rotations,
		                                const Slice1D<int> bone_parents,
		                                const int bone);
        void NormalizeFeature(Slice2D<float> features,
                              Slice1D<float> features_offset,
                              Slice1D<float> features_scale,
		                      const int offset,
		                      const int size,
		                      const float weight);


		void RecursiveForwardKinematicsVelocity(Piccolo::Vector3& bone_position,
		                                        Piccolo::Vector3& bone_velocity,
		                                        Piccolo::Quaternion& bone_rotation,
		                                        Piccolo::Vector3& bone_angular_velocity,
		                                        const Slice1D<Piccolo::Vector3> bone_positions,
		                                        const Slice1D<Piccolo::Vector3> bone_velocities,
		                                        const Slice1D<Piccolo::Quaternion> bone_rotations,
		                                        const Slice1D<Piccolo::Vector3> bone_angular_velocities,
		                                        const Slice1D<int> bone_parents,
		                                        const int bone);

		int DatabaseTrajectoryIndexClamp(CDataBase& db, int frame, int offset);
		void trajectory_desired_rotations_predict(Slice1D<Piccolo::Quaternion> desired_rotations,
		                                          const Slice1D<Piccolo::Vector3> desired_velocities,
		                                          const Piccolo::Quaternion desired_rotation,
		                                          const float camera_azimuth,
		                                          const Piccolo::Vector3 gamepadstick_left,
		                                          const Piccolo::Vector3 gamepadstick_right,
		                                          const bool desired_strafe,
		                                          const float dt);
		float orbit_camera_update_azimuth(float azimuth, Piccolo::Vector3 gamepadstick_right, bool desired_strafe, float dt);
		void trajectory_rotations_predict(Slice1D<Piccolo::Quaternion> rotations,
		                                  Slice1D<Piccolo::Vector3> angular_velocities,
		                                  const Piccolo::Quaternion rotation,
		                                  const Piccolo::Vector3 angular_velocity,
		                                  const Slice1D<Piccolo::Quaternion> desired_rotations,
		                                  const float halflife,
		                                  const float dt);
		void simulation_rotations_update(Piccolo::Quaternion& rotation,
		                                 Piccolo::Vector3& angular_velocity,
		                                 const Piccolo::Quaternion desired_rotation,
		                                 const float halflife,
		                                 const float dt);
		void trajectory_desired_velocities_predict(Slice1D<Piccolo::Vector3> desired_velocities,
		                                           const Slice1D<Piccolo::Quaternion> trajectory_rotations,
		                                           const Piccolo::Vector3 desired_velocity,
		                                           const float camera_azimuth,
		                                           const Piccolo::Vector3 gamepadstick_left,
		                                           const Piccolo::Vector3 gamepadstick_right,
		                                           const bool desired_strafe,
		                                           const float fwrd_speed,
		                                           const float side_speed,
		                                           const float back_speed,
		                                           const float dt);

		void trajectory_positions_predict(Slice1D<Piccolo::Vector3> positions,
		                                  Slice1D<Piccolo::Vector3> velocities,
		                                  Slice1D<Piccolo::Vector3> accelerations,
		                                  const Piccolo::Vector3 position,
		                                  const Piccolo::Vector3 velocity,
		                                  const Piccolo::Vector3 acceleration,
		                                  const Slice1D<Piccolo::Vector3> desired_velocities,
		                                  const float halflife,
		                                  const float dt);

		void simulation_positions_update(Piccolo::Vector3& position,
		                                 Piccolo::Vector3& velocity,
		                                 Piccolo::Vector3& acceleration,
		                                 const Piccolo::Vector3 desired_velocity,
		                                 const float halflife,
		                                 const float dt);
		void query_copy_denormalized_feature(Slice1D<float> query,
		                                     int& offset,
		                                     const int size,
		                                     const Slice1D<float> features,
		                                     const Slice1D<float> features_offset,
		                                     const Slice1D<float> features_scale);

		void query_compute_trajectory_position_feature(Slice1D<float> query,
		                                               int& offset,
		                                               const Piccolo::Vector3 root_position,
		                                               const Piccolo::Quaternion root_rotation,
		                                               const Slice1D<Piccolo::Vector3> trajectory_positions);

		void query_compute_trajectory_direction_feature(Slice1D<float> query,
		                                                int& offset,
		                                                const Piccolo::Quaternion root_rotation,
		                                                const Slice1D<Piccolo::Quaternion> trajectory_rotations);
		int database_trajectory_index_clamp(CDataBase& db, int frame, int offset);
		void forward_kinematic_full();
		void inertialize_pose_reset(Slice1D<Piccolo::Vector3> bone_offset_positions,
		                            Slice1D<Piccolo::Vector3> bone_offset_velocities,
		                            Slice1D<Piccolo::Quaternion> bone_offset_rotations,
		                            Slice1D<Piccolo::Vector3> bone_offset_angular_velocities,
		                            Piccolo::Vector3& transition_src_position,
		                            Piccolo::Quaternion& transition_src_rotation,
		                            Piccolo::Vector3& transition_dst_position,
		                            Piccolo::Quaternion& transition_dst_rotation,
		                            const Piccolo::Vector3 root_position,
		                            const Piccolo::Quaternion root_rotation);
		void inertialize_pose_transition(Slice1D<Piccolo::Vector3> bone_offset_positions,
		                                 Slice1D<Piccolo::Vector3> bone_offset_velocities,
		                                 Slice1D<Piccolo::Quaternion> bone_offset_rotations,
		                                 Slice1D<Piccolo::Vector3> bone_offset_angular_velocities,
		                                 Piccolo::Vector3& transition_src_position,
		                                 Piccolo::Quaternion& transition_src_rotation,
		                                 Piccolo::Vector3& transition_dst_position,
		                                 Piccolo::Quaternion& transition_dst_rotation,
		                                 const Piccolo::Vector3 root_position,
		                                 const Piccolo::Vector3 root_velocity,
		                                 const Piccolo::Quaternion root_rotation,
		                                 const Piccolo::Vector3 root_angular_velocity,
		                                 const Slice1D<Piccolo::Vector3> bone_src_positions,
		                                 const Slice1D<Piccolo::Vector3> bone_src_velocities,
		                                 const Slice1D<Piccolo::Quaternion> bone_src_rotations,
		                                 const Slice1D<Piccolo::Vector3> bone_src_angular_velocities,
		                                 const Slice1D<Piccolo::Vector3> bone_dst_positions,
		                                 const Slice1D<Piccolo::Vector3> bone_dst_velocities,
		                                 const Slice1D<Piccolo::Quaternion> bone_dst_rotations,
		                                 const Slice1D<Piccolo::Vector3> bone_dst_angular_velocities);

		void inertialize_pose_update(Slice1D<Piccolo::Vector3> bone_positions,
		                             Slice1D<Piccolo::Vector3> bone_velocities,
		                             Slice1D<Piccolo::Quaternion> bone_rotations,
		                             Slice1D<Piccolo::Vector3> bone_angular_velocities,
		                             Slice1D<Piccolo::Vector3> bone_offset_positions,
		                             Slice1D<Piccolo::Vector3> bone_offset_velocities,
		                             Slice1D<Piccolo::Quaternion> bone_offset_rotations,
		                             Slice1D<Piccolo::Vector3> bone_offset_angular_velocities,
		                             const Slice1D<Piccolo::Vector3> bone_input_positions,
		                             const Slice1D<Piccolo::Vector3> bone_input_velocities,
		                             const Slice1D<Piccolo::Quaternion> bone_input_rotations,
		                             const Slice1D<Piccolo::Vector3> bone_input_angular_velocities,
		                             const Piccolo::Vector3 transition_src_position,
		                             const Piccolo::Quaternion transition_src_rotation,
		                             const Piccolo::Vector3 transition_dst_position,
		                             const Piccolo::Quaternion transition_dst_rotation,
		                             const float halflife,
		                             const float dt);
        void denormalize_features(Slice1D<float>       features,
                                  const Slice1D<float> features_offset,
                                  const Slice1D<float> features_scale);
        void DoRetargeting();
	public:
// #if _DEBUG
		// std::vector<Piccolo::Vector3> m_current_feature_positions;
		// std::vector<Piccolo::Quaternion> m_current_feature_rotations;
		// Piccolo::Quaternion m_current_root_rotation{Piccolo::Quaternion::IDENTITY};
		//
		// std::vector<Piccolo::Vector3> m_matched_feature_positions;
		// std::vector<Piccolo::Vector3> m_matched_feature_direction;
// #endif
	};
}
