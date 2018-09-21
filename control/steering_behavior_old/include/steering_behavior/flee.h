#include "steering_behavior/steering_behavior.h"
#include <geometry_msgs/Point32.h>
#include <geometry_msgs/Vector3.h>

using SteeringBehavior::Flee;

class Flee: public SteeringBehavior {
	private:
		float safety_distance_;
	public:
		Flee(geometry_msgs::Point32 target_):SteeringBehavior(target_){}

		geometry_msgs::Vector3 calculate_desired_velocity(geometry_msgs::Point32 current_pos);
        geometry_msgs::Vector3 calculate_steering_force(geometry_msgs::Vector3 current_vel, geometry_msgs::Vector3 desired_vel);

		bool evaluateSafety(geometry_msgs::Point32 current_pos);
};