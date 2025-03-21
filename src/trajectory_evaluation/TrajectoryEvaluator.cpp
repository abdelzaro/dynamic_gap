#include <dynamic_gap/trajectory_evaluation/TrajectoryEvaluator.h>


namespace dynamic_gap 
{
    TrajectoryEvaluator::TrajectoryEvaluator(const dynamic_gap::DynamicGapConfig& cfg)
    {
        cfg_ = & cfg;
    }

    void TrajectoryEvaluator::updateEgoCircle(boost::shared_ptr<sensor_msgs::LaserScan const> scan) 
    {
        boost::mutex::scoped_lock lock(scanMutex_);
        scan_ = scan;
    }

    void TrajectoryEvaluator::transformGlobalPathLocalWaypointToRbtFrame(const geometry_msgs::PoseStamped & globalPathLocalWaypointOdomFrame, 
                                                                            const geometry_msgs::TransformStamped & odom2rbt) 
    {
        boost::mutex::scoped_lock lock(globalPlanMutex_);
        tf2::doTransform(globalPathLocalWaypointOdomFrame, globalPathLocalWaypointRobotFrame_, odom2rbt);
    }

    void TrajectoryEvaluator::evaluateTrajectory(const dynamic_gap::Trajectory & traj,
                                                std::vector<float> & posewiseCosts,
                                                float & terminalPoseCost,
                                                const std::vector<sensor_msgs::LaserScan> & futureScans, 
                                                const dynamic_gap::Gap* gap, 
                                                int i)  //TODO: get rid of i it's just ofr debugging 
    {    
        ROS_INFO_STREAM_NAMED("GapTrajectoryGenerator", "         [evaluateTrajectory()]");
        // ROS_ERROR_STREAM_NAMED("GapTrajectoryGenerator", i);
        
        Eigen::Vector2f leftGapRelVel(0, 0);
        Eigen::Vector2f RbtVel(0, 0);
        Eigen::Vector2f leftGapRelPos(0, 0);
        Eigen::Vector2f rightGapRelVel(0, 0);
        Eigen::Vector2f rightGapRelPos(0, 0);
        bool leftPtIsStatic = true; 
        bool rightPtIsStatic = true; 

        if(gap){ // if a gap is provided 
        gap->leftGapPtModel_->updateGapPtIsStatic();
        gap->rightGapPtModel_->updateGapPtIsStatic();
        leftPtIsStatic = gap->leftGapPtModel_->getGapPtIsStatic();
        rightPtIsStatic = gap->rightGapPtModel_->getGapPtIsStatic();

        ROS_ERROR_STREAM_NAMED(" ", "gap->leftGapPtModel_->getGapPtIsStatic()");
        ROS_ERROR_STREAM_NAMED(" ", gap->leftGapPtModel_->getGapPtIsStatic());

        ROS_ERROR_STREAM_NAMED(" ", "gap->rightGapPtModel_->getGapPtIsStatic()");
        ROS_ERROR_STREAM_NAMED(" ", gap->rightGapPtModel_->getGapPtIsStatic());

        if(!leftPtIsStatic)
        {
        gap->leftGapPtModel_->isolateGapDynamics();
         leftGapRelVel = gap->leftGapPtModel_->getGapVelocity();
         RbtVel = gap->leftGapPtModel_->getRbtVel();
        // ROS_ERROR_STREAM_NAMED("evalTraj", "leftGapRelVel: ");
        // ROS_ERROR_STREAM_NAMED("evalTraj", leftGapRelVel);  
         leftGapRelPos = gap->leftGapPtModel_->getState().head<2>(); //distance from robot to gap.
        // ROS_ERROR_STREAM_NAMED("evalTraj", "gap->leftGapPtModel_->getState(): ");
        // ROS_ERROR_STREAM_NAMED("evalTraj", leftGapRelPos);  

        // Eigen::Vector2f leftGapRelVel = RbtVel + leftGapRelVel;// TODO: delete this it's unused
        }

        if(!rightPtIsStatic)
        {
        gap->rightGapPtModel_->isolateGapDynamics();
        rightGapRelVel = gap->rightGapPtModel_->getGapVelocity();
        // ROS_ERROR_STREAM_NAMED("evalTraj", "rightGapRelVel: ");
        // ROS_ERROR_STREAM_NAMED("evalTraj", rightGapRelVel);
        rightGapRelPos = gap->rightGapPtModel_->getState().head<2>(); //distance from robot to gap.
        // ROS_ERROR_STREAM_NAMED("evalTraj", "gap->rightGapPtModel_->getState(): ");
        // ROS_ERROR_STREAM_NAMED("evalTraj", rightGapRelPos);  

        // Eigen::Vector2f rightGapRelVel = RbtVel + rightGapRelVel; // TODO: delete this it's unused

        // Eigen::Vector2f avgGapRelVel = (leftGapRelVel + rightGapRelVel)/2;// TODO: delete this it's unused


       
        // ROS_ERROR_STREAM_NAMED("evalTraj", "(leftGapRelVel + rightGapRelVel)/2");         
        // ROS_ERROR_STREAM_NAMED("evalTraj", avgGapRelVel);         



        // if (i==1){
        // ROS_ERROR_STREAM_NAMED("evalTraj", "for i=1 only: evaluateTrajectory containing RbtVel + gapVelocity: "); 
        // ROS_ERROR_STREAM_NAMED("evalTraj", leftGapRelVel);
        // }
        // // ROS_ERROR_STREAM_NAMED("evalTraj", "perfect estimator used, evaluateTrajectory containing gapVelocity: "); 
        // // ROS_ERROR_STREAM_NAMED("evalTraj", gapVelocity); 
        }
        }
        
        // Requires LOCAL FRAME
        geometry_msgs::PoseArray path = traj.getPathRbtFrame();
        std::vector<float> pathTiming = traj.getPathTiming();
        
        posewiseCosts = std::vector<float>(path.poses.size());
        float leftGapCost = 0; 
        float rightGapCost = 0; 
        float relVelWeight = .1; //TODO: add this to cfg
        for (int i = 0; i < posewiseCosts.size(); i++) 
        {
            // std::cout << "regular range at " << i << ": ";
            
            if(!leftPtIsStatic){leftGapCost = relativeVelocityCost(leftGapRelVel, leftGapRelPos, RbtVel);}
            if(!rightPtIsStatic){rightGapCost = relativeVelocityCost(rightGapRelVel, rightGapRelPos, RbtVel);}
        
            posewiseCosts.at(i) = evaluatePose(path.poses.at(i), futureScans.at(i)) + relVelWeight * leftGapCost + relVelWeight * rightGapCost; //  / posewiseCosts.size()
            // ROS_ERROR_STREAM_NAMED("GapTrajectoryGenerator", "left leftGapCost: ");
            // ROS_ERROR_STREAM_NAMED("GapTrajectoryGenerator", leftGapCost);

            // ROS_ERROR_STREAM_NAMED("GapTrajectoryGenerator", "right rightGapCost: ");
            // ROS_ERROR_STREAM_NAMED("GapTrajectoryGenerator", rightGapCost);



            // ROS_ERROR_STREAM_NAMED("GapTrajectoryGenerator", "leftGapCost to evaluatePose() ");

            // ROS_ERROR_STREAM_NAMED("GapTrajectoryGenerator", "           pose " << i << " score: " << posewiseCosts.at(i));

        }
        float totalTrajCost = std::accumulate(posewiseCosts.begin(), posewiseCosts.end(), float(0));
        ROS_INFO_STREAM_NAMED("GapTrajectoryGenerator", "             pose-wise cost: " << totalTrajCost);

        if (posewiseCosts.size() > 0) 
        {
            // obtain terminalGoalCost, scale by Q
            terminalPoseCost = cfg_->traj.Q_f * terminalGoalCost(*std::prev(path.poses.end()));

            ROS_INFO_STREAM_NAMED("GapTrajectoryGenerator", "            terminal cost: " << terminalPoseCost);
        }
        
        // ROS_INFO_STREAM_NAMED("TrajectoryEvaluator", "evaluateTrajectory time taken:" << ros::WallTime::now().toSec() - start_time);
        return;
    }

    float TrajectoryEvaluator::terminalGoalCost(const geometry_msgs::Pose & pose) 
    {
        boost::mutex::scoped_lock planlock(globalPlanMutex_);
        // ROS_INFO_STREAM_NAMED("TrajectoryEvaluator", pose);
        ROS_INFO_STREAM_NAMED("GapTrajectoryGenerator", "            final pose: (" << pose.position.x << ", " << pose.position.y << "), local goal: (" << globalPathLocalWaypointRobotFrame_.pose.position.x << ", " << globalPathLocalWaypointRobotFrame_.pose.position.y << ")");
        float dx = pose.position.x - globalPathLocalWaypointRobotFrame_.pose.position.x;
        float dy = pose.position.y - globalPathLocalWaypointRobotFrame_.pose.position.y;
        return sqrt(pow(dx, 2) + pow(dy, 2));
    }

    float TrajectoryEvaluator::evaluatePose(const geometry_msgs::Pose & pose,
                                      const sensor_msgs::LaserScan scan_k) 
    {
        boost::mutex::scoped_lock lock(scanMutex_);
        // sensor_msgs::LaserScan scan = *scan_.get();

        // obtain orientation and idx of pose

        // dist is size of scan
        std::vector<float> scan2RbtDists(scan_k.ranges.size());

        // iterate through ranges and obtain the distance from the egocircle point and the pose
        // Meant to find where is really small
        // float currScan2RbtDist = 0.0;
        for (int i = 0; i < scan2RbtDists.size(); i++) 
        {
            scan2RbtDists.at(i) = dist2Pose(idx2theta(i), scan_k.ranges.at(i), pose);
        }

        auto iter = std::min_element(scan2RbtDists.begin(), scan2RbtDists.end());
        // std::cout << "robot pose: " << pose.position.x << ", " << pose.position.y << ")" << std::endl;
        int minDistIdx = std::distance(scan2RbtDists.begin(), iter);
        float range = scan_k.ranges.at(minDistIdx);
        float theta = idx2theta(minDistIdx);
        float cost = chapterCost(*iter);
        //std::cout << *iter << ", regular cost: " << cost << std::endl;
        ROS_INFO_STREAM_NAMED("TrajectoryEvaluator", "            robot pose: " << pose.position.x << ", " << pose.position.y << 
                    ", closest scan point: " << range * std::cos(theta) << ", " << range * std::sin(theta) << ", static cost: " << cost);
        return cost;
    }

    float TrajectoryEvaluator::chapterCost(const float & rbtToScanDist) 
    {
        // if the distance at the pose is less than the inscribed radius of the robot, return negative infinity
        // std::cout << "in chapterCost with distance: " << d << std::endl;
        float inflRbtRad = cfg_->rbt.r_inscr * cfg_->traj.inf_ratio; 

        float inflRbtToScanDist = rbtToScanDist - inflRbtRad;

        if (inflRbtToScanDist < 0.0) 
        {   
            return std::numeric_limits<float>::infinity();
        }

        // if pose is sufficiently far away from scan, return no cost
        if (rbtToScanDist > cfg_->traj.max_pose_to_scan_dist) 
            return 0;

        /*   y
         *   ^
         * Q |\ 
         *   | \
         *   |  \
         *       `
         *   |    ` 
         *         ` ---
         *   |          _________
         *   --------------------->x
         *
         */


        return cfg_->traj.Q * std::exp(-cfg_->traj.pen_exp_weight * inflRbtToScanDist);
    }

    float TrajectoryEvaluator::relativeVelocityCost(Eigen::Vector2f relativeVel,
                                                Eigen::Vector2f relativeGapPos,
                                                Eigen::Vector2f robotVel)
    {
        Eigen::Vector2f relVel = -relativeVel; // so velocity and position vector point in the same direction for the dot product
        
        // ROS_ERROR_STREAM_NAMED("relvel cost: ", relVel << ", relativeGapPos: " << relativeGapPos << ", robotVel: " << robotVel);

        float cost = (std::max(relVel.dot(relativeGapPos), 0.0f) + robotVel.norm() + 1) / relativeGapPos.norm();
        
        // ROS_ERROR_STREAM_NAMED("relvel cost", "std::max(relVel.dot(relativeGapPos), 0.0f");
        // ROS_ERROR_STREAM_NAMED("relvel cost", std::max(relVel.dot(relativeGapPos), 0.0f));

        // ROS_ERROR_STREAM_NAMED("relvel cost", "relVel.dot(relativeGapPos)");
        // ROS_ERROR_STREAM_NAMED("relvel cost", relVel.dot(relativeGapPos));
        
        // ROS_ERROR_STREAM_NAMED("GapTrajectoryGenerator", "relativeVelocityCost() !!UNWEIGHTED!! cost: ");
        // ROS_ERROR_STREAM_NAMED("GapTrajectoryGenerator", cost);

    return cost;
    }


}