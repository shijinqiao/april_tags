/**
 * 
 */

#include <ros/ros.h>
#include <tf/transform_broadcaster.h>
#include <tf/transform_listener.h>
#include <nav_msgs/Odometry.h>
#include "std_msgs/Float32.h"
#include "AprilTags/AprilTagReader.h"

#define MAX_NUM_APRIL_TAGS 10

ros::Time last_lw_time, last_rw_time;

bool hasNewSpeeds = true;

void sendTags()

    for (int i=0; i<MAX_NUM_APRIL_TAGS; i++)
    {
      stringstream ss;
      ss<<"april_tag["<<i<<"]";
      if (listener.canTransform ("/map", ss.str().c_str(), ros::Time(0)))
      {
        listener.lookupTransform("/map", ss.str().c_str(),
                                 ros::Time(0), transform);
        quat = transform.getRotation();

        ps.header.frame_id = itoa(i);
        ps.pose.position.x = transform.getOrigin().x();
        ps.pose.position.y = transform.getOrigin().y();

        geometry_msgs::Quaternion q = createQuaternionMsgFromRollPitchYaw(quat.x(),quat.y(),quat.z());
        ps.pose.orientation = q;
        tags_pub.publish(ps);
      }
    }
}

int main(int argc, char** argv){
  ros::init(argc, argv, "AprilTagsNode");

  ros::NodeHandle n;

  /*
  std::vector<tf::TransformBroadcaster> tags_broadcaster;
  for (int i = 0; i < MAX_NUM_APRIL_TAGS; ++i)
  {
    tags_broadcaster.push_back(tf::TransformBroadcaster());
  }
  */
 

  tf::TransformBroadcaster tags_broadcaster;

  ros::Publisher tags_pub = n.advertise<geometry_msgs::PoseStamped>("/apriltags", 1000);

  AprilTagReader reader;
  reader.setup();

  ros::Time current_time, last_time;
  current_time = ros::Time::now();
  last_time = ros::Time::now();

  ros::Rate r(1.0);

  tf::TransformListener listener;

  while(n.ok()){
    ros::spinOnce();               // check for incoming messages

    ROS_INFO("Reading April Tags...");
    reader.read();

    current_time = ros::Time::now();

    ROS_INFO("Running... #tags: %ld, time: %lf",reader.getTags().size(), current_time.toSec());

    for (int i=0; i<reader.getTags().size(); i++)
    {
      AprilTags::TagDetection td = reader.getTags()[i];
      int id = reader.getTags()[i].id;

      double x,y,z,roll,pitch,yaw;
      
      reader.getTransformInfo(td, x,y,z,roll,pitch,yaw);

      //tf::Pose pose = GetPoseFromTagDetection(td, reader, "/camera_link");

      //Pitch = z axis
      //Yaw = x axis
      //Roll = y axis

      //pitch += PI;
      //pitch = -pitch;

      geometry_msgs::Quaternion odom_quat = tf::createQuaternionMsgFromRollPitchYaw(yaw, roll, pitch);

      //first, we'll publish the transform over tf
      geometry_msgs::TransformStamped tag_transform;

      tag_transform.header.stamp = current_time;
      tag_transform.header.frame_id = "camera_link";

      stringstream ss;
      ss << "/april_tag["<<id<<"]";
      
      tag_transform.child_frame_id = ss.str().c_str();

      tag_transform.transform.translation.x = x;
      tag_transform.transform.translation.y = y;
      tag_transform.transform.translation.z = z;
      tag_transform.transform.rotation = odom_quat;

      //send the transform
      tags_broadcaster.sendTransform(tag_transform);

      ///trash can has been found. send map->tag transform
      if (id % 2 == 0)
      {
        tf::StampedTransform transform;
        
        geometry_msgs::PoseStamped ps;
        tf::Quaternion quat;

        listener.lookupTransform("/map", "/april_tag[6]",
                                 ros::Time(0), transform);
        quat = transform.getRotation();
      
        ps.header.frame_id = "6";
        ps.pose.position.x = transform.getOrigin().x();
        ps.pose.position.y = transform.getOrigin().y();
        geometry_msgs::Quaternion q;
        q.x=0;
        q.y=0;
        q.z=quat.z();
        ps.pose.orientation = q;
        tags_pub.publish(ps);
      }
    }

    cvWaitKey(10);
    last_time = current_time;
    r.sleep();
  }
}
