#include <ros/ros.h>
#include <cmath>
#include "planning/action_planning.h"
#include "planning/ActionEncoded.h"
#include "planning/SafetyMsg.h"
#include <behavior_with_deception/DeceptionCommandMsg.h>

#define CAPTURE_TOWER 1
#define ESCAPE 2
#define DECEIVE 3

ActionPlanning::ActionPlanner::ActionPlanner(){
    ROS_INFO("Creating action planner...");

    if (!nh_.getParam("/planning/action_topic", action_topic_)){
            ROS_ERROR("ACTION PLANNER: could not read 'action_topic' from rosparam!");
            exit(-1);
    }
    if (!nh_.getParam("/planning/safety_topic", safety_topic_)){
        ROS_ERROR("ACTION PLANNER: could not read 'safety_topic' from rosparam!");
        exit(-1);
    }

    if (!nh_.getParam("/planning/deception_command_topic", deception_command_topic_)){
        ROS_ERROR("ACTION PLANNER: could not read 'deception_command_topic' from rosparam!");
        exit(-1);
    }
    
    action_pub_ = nh_.advertise<planning::ActionEncoded>(action_topic_, 1);
    safety_sub_ = nh_.subscribe(safety_topic_, 1, &ActionPlanning::ActionPlanner::safetyCallback, this);
    deception_command_sub_ = nh_.subscribe(deception_command_topic_, 1, &ActionPlanning::ActionPlanner::deceptionCommandCallback, this);

    is_first_deceptive_message_ = false;

    last_action_ = planning::ActionEncoded();
    last_action_.action_code = -1; // Initializing to invalid code

    priority_ = 0;
    safety_count_ = 0;

    
}

void ActionPlanning::ActionPlanner::safetyCallback(const planning::SafetyMsg& msg){
    safety_msg_ = msg;

    if(safety_msg_.safety){
        if(safety_count_ < 5){
            safety_count_++;
        }
    }
    else{
        safety_count_ = 0;
    }
}

void ActionPlanning::ActionPlanner::deceptionCommandCallback(const behavior_with_deception::DeceptionCommandMsg& msg){
    ROS_INFO("GETTING DECEIVING COMMAND");
    deceiving_command_msg_ = msg;
    // Discarding IDLE and CENTER messages: NOT IMPLEMENTED YET
    if(deceiving_command_msg_.type == -1 or deceiving_command_msg_.type == -2){
        return;
    }
    if(!currently_being_deceptive_){
        currently_being_deceptive_ = true;
    }
}

void ActionPlanning::ActionPlanner::updateLoop(){
    planning::ActionEncoded action_msg;

    bool is_safe = safety_count_ >= 5;
    if(!is_safe){
        // ROS_WARN("ESCAPING");
        currently_being_deceptive_ = false;
        is_first_deceptive_message_ = true;
        action_msg.priority = priority_;
        action_msg.action_name = "escape";
        action_msg.action_code = ESCAPE;
        // Encoding information about the point to flee away
        action_msg.danger_sector = safety_msg_.section_index;
        action_msg.danger_index = safety_msg_.danger_index;        
    }
    else if(currently_being_deceptive_){
        // ROS_WARN("DECEIVING");
        if(is_first_deceptive_message_){
            is_first_deceptive_message_ = false;
            action_msg.priority = priority_;
            action_msg.action_name = "deceive";
            action_msg.action_code = DECEIVE;
            // Encoding targets received from the deception planner
            action_msg.real_target = deceiving_command_msg_.real_target;
            action_msg.fake_target = deceiving_command_msg_.fake_target;
            action_msg.type = deceiving_command_msg_.type;
        }

    }
    else{
        // ROS_WARN("CAPTURING TOWER");
        action_msg.priority = priority_;
        action_msg.action_name = "capture_tower";
        action_msg.action_code = CAPTURE_TOWER;
    }

    if(action_msg.action_code != last_action_.action_code){
        // If we change our intention, we publish it
        // Update last_action with current action
        ROS_WARN_STREAM(action_msg.action_name);
        last_action_ = action_msg;
        priority_ = priority_ + 1;        
    }   
        action_pub_.publish(action_msg);

}

int main(int argc, char** argv){
    ros::init(argc, argv, "planning_action_node");
    ros::NodeHandle nh;
    ros::Rate rate(30);

    ROS_INFO("The Action Planner will wait 3secs before starting!");
    ros::Duration(3.0).sleep(); // sleep for 5 seconds before beginning.

    ActionPlanning::ActionPlanner planner;

    while(ros::ok()){
        planner.updateLoop();
        ros::spinOnce();
        rate.sleep();
    }
}
