#include "hybird_A_star.h"

HybirdAStar::HybirdAStar()
: start_x_(0), start_y_(0), start_theta_(0), obstacle_threshold_(65), find_path_flag_(0) {
    sub_ori_map_ = nh_.subscribe("/map", 2, &HybirdAStar::subOriMap, this);
    sub_start_pose_ = nh_.subscribe("/initialpose", 2, &HybirdAStar::subStartPose, this);
    sub_goal_pose_ = nh_.subscribe("/move_base_simple/goal", 2, &HybirdAStar::subGoalPose, this);
    pub_tree_ = nh_.advertise<sensor_msgs::PointCloud>("/tree", 3);
    pub_path_ = nh_.advertise<nav_msgs::Path>("/own_path", 3);
    w_seq_.push_back(20);
    w_seq_.push_back(10);
    w_seq_.push_back(5);
    w_seq_.push_back(0);
    w_seq_.push_back(-5);
    w_seq_.push_back(-10);
    w_seq_.push_back(-20);
    ros::Duration(1).sleep();

    while(ros::ok()) {
        ros::spin();
    }

    ///////////////////////
    int mx;
    int my;
    worldToMap(-6, -5, mx, my);
    cout << mx << ", " << my << endl;
    ///////////////////////
}

HybirdAStar::~HybirdAStar() {}

void HybirdAStar::subOriMap(nav_msgs::OccupancyGrid map) {
    ori_map_ = map;
    extend_dist_ = ori_map_.info.resolution * 1.414;
    cout << "get the origin map." << endl;
    generate_paths_.loadTheMap(map);
}

void HybirdAStar::subStartPose(geometry_msgs::PoseWithCovarianceStamped start_pose) {
    start_x_ = start_pose.pose.pose.position.x;
    start_y_ = start_pose.pose.pose.position.y;
    start_theta_ = acos(2 * pow(start_pose.pose.pose.orientation.w, 2) - 1);
    start_theta_ = start_pose.pose.pose.orientation.w * start_pose.pose.pose.orientation.z < 0 ? - start_theta_ : start_theta_;
    //cout << start_x_ << ", " << start_y_ << ", " << start_theta_ << endl;
}

void HybirdAStar::subGoalPose(geometry_msgs::PoseStamped goal_pose) {
    goal_x_ = goal_pose.pose.position.x;
    goal_y_ = goal_pose.pose.position.y;
    goal_theta_ = acos(2 * pow(goal_pose.pose.orientation.w, 2) - 1);
    goal_theta_ = goal_pose.pose.orientation.w * goal_pose.pose.orientation.z < 0 ? - goal_theta_ : goal_theta_;
    //cout << goal_x_ << ", " << goal_y_ << ", " << goal_theta_ << endl;
    for(int i = 0; i < tree_.size(); i++) {
        delete tree_[i];
    }
    tree_.clear();
    PathNode* one_path_node;
    Path path;
    path.path_.push_back(Node(start_x_, start_y_, start_theta_));
    path.length_ = 0;
    one_path_node = new PathNode(path, 0, NULL);
    tree_.push_back(one_path_node);
    searchThePath();

    cout << "tree size = " << tree_.size() << endl;
}

void HybirdAStar::worldToMap(double wx, double wy, int& mx, int& my) {
    mx = (wx - ori_map_.info.origin.position.x) / ori_map_.info.resolution;
    my = (wy - ori_map_.info.origin.position.y) / ori_map_.info.resolution;
}

bool HybirdAStar::nodeEquality(Node* node1, Node* node2) {
    int mx1;
    int my1;
    int mx2;
    int my2;
    worldToMap(node1->x_, node1->y_, mx1, my1);
    worldToMap(node2->x_, node2->y_, mx2, my2);
    if((mx1 == mx2) && (my1 == my2)) {
        return true;
    }
    else {
        return false;
    }
}

bool HybirdAStar::searchThePath() {
    bool ans;
    //generate_paths_.generatePaths(start_x_, start_y_, goal_x_, goal_y_, ori_map_.info.resolution);
    //generate_paths_.generatePaths(start_x_, start_y_, start_theta_, goal_x_, goal_y_, ori_map_.info.resolution);
    cout << "test: " << tree_.size() << endl;
    Path path = generate_paths_.generatePaths(start_x_, start_y_, goal_x_, goal_y_, ori_map_.info.resolution);
    if(path.path_.size() != 0) {
        PathNode* one_path_node;
        one_path_node = new PathNode(path, 0, tree_[0]);
        tree_.push_back(one_path_node);
        displayTheTree();
        return true;
    }
    extendTreeRoot();

    return false;
}

bool HybirdAStar::nodeObstacleCheck(Node* node) {
    int mx;
    int my;
    worldToMap(node->x_, node->y_, mx, my);
    if(ori_map_.data[mx + my * ori_map_.info.width] < obstacle_threshold_) {
        return false;
    }
    else {
        return true;
    }
}

bool HybirdAStar::nodeObstacleCheck(double x, double y) {
    int mx;
    int my;
    worldToMap(x, y, mx, my);
    int test = ori_map_.data[mx + my * ori_map_.info.width];
    if(ori_map_.data[mx + my * ori_map_.info.width] < obstacle_threshold_ && ori_map_.data[mx + my * ori_map_.info.width] != -1) {
        return false;
    }
    else {
        return true;
    }
}

void HybirdAStar::extendTree(Node* node) {
}

void HybirdAStar::extendTreeRoot() {
    for(double theta = -PI; theta < PI; theta += PI / 20) {
        PathNode* one_path_node;
        Path one_path;
        Node one_node;
        double c = cos(theta);
        double s = sin(theta);
        for(double dist = 0; dist < extend_dist_; dist += ori_map_.info.resolution) {
            one_node.x_ = tree_[0]->path_.path_[0].x_ + dist * c;
            one_node.y_ = tree_[0]->path_.path_[0].y_ + dist * s;
            if(nodeObstacleCheck(one_node.x_, one_node.y_) == true) {
                one_path.path_.clear();
                break;
            }
            else {
                one_path.path_.push_back(one_node);
            }
        }
        if(one_path.path_.size() == 0) {
            continue;
        }
        else {
            double h = tree_[0]->length_ + extend_dist_ + hypot((one_path.path_.end() - 1)->x_ - goal_x_, (one_path.path_.end() - 1)->y_ - goal_y_);
            one_path.length_ = extend_dist_;
            one_path_node = new PathNode(one_path, h, tree_[0]);
            tree_.push_back(one_path_node);
        }
    }
}

int HybirdAStar::beInTree(Node* node, vector<Node*>& tree) {
    for(int i = 0; i < tree.size(); i++) {
        if(nodeEquality(node, tree[i])) {
            return i;
        }
    }
    return -1;
}

double HybirdAStar::deltaAngle(double angle0, double angle1) {
    double ans;
    ans = angle1 - angle0;
    ans = ans < -PI ? ans + 2 * PI : ans;
    ans = ans > PI ? ans - 2 * PI : ans;

    return ans;
}

double HybirdAStar::vectorProgection(double base_x, double base_y, double x, double y) {
    return (base_x * x + base_y * y) / hypot(base_x, base_y);
}

bool HybirdAStar::findPathCheck(double x, double y) {
    int x0;
    int y0;
    int x1;
    int y1;
    worldToMap(x, y, x0, y0);
    worldToMap(goal_x_, goal_y_, x1, y1);
    if((x0 == x1) && (y0 == y1)) {
        find_path_flag_ = true;
        return true;
    }
    return false;
}

void HybirdAStar::displayTheTree() {
    sensor_msgs::PointCloud points;
    points.header.frame_id = "map";
    for(int i = 1; i < tree_.size(); i++) {
        for(int j = 0; j < tree_[i]->path_.path_.size(); j++) {
            geometry_msgs::Point32 point;
            point.x = tree_[i]->path_.path_[j].x_;
            point.y = tree_[i]->path_.path_[j].y_;
            point.z = 0.01;
            points.points.push_back(point);
        }
    }
    pub_tree_.publish(points);
    //getchar();
}

void HybirdAStar::displayThePath() {
    nav_msgs::Path path;
    path.header.frame_id = "map";

    pub_path_.publish(path);
    cout << "display the path." << endl;
}

int HybirdAStar::bestSearchNode() {
    int index;
    double value = 9999999999;
    for(int i = 0; i < open_list_.size(); i++) {
    }
    return index;
}

int main(int argc, char** argv) {
    cout << "begin the program." << endl;
    ros::init(argc, argv, "hybird_A_star");
    HybirdAStar test;
    return 0;
}