#ifndef GENERATE_PATHS_H
#define GENERATE_PATHS_H

#include <iostream>
#include <vector>

#include <ros/ros.h>
#include <nav_msgs/Path.h>

using namespace std;

class Node {
public:
    double x_;
    double y_;
    double theta_;
    double v_;
    double w_;
    Node* father_node_;
    double heuristics_value_;

    Node();
    ~Node() {};
    Node(double x, double y, double theta) {
        x_ = x;
        y_ = y;
        theta_ = theta;
        heuristics_value_ = 0;
    };
    Node(double x, double y, double theta, double v, double w, double heuristics_value) {
        x_ = x;
        y_ = y;
        theta_ = theta;
        v_ = v;
        w_ = w;
        heuristics_value_ = heuristics_value;
    };

    bool operator<(Node* node) {
        return heuristics_value_ < node->heuristics_value_;
    };

    void copy(Node* node0) {
        x_ = node0->x_;
        y_ = node0->y_;
        theta_ = node0->theta_;
        v_ = node0->v_;
        w_ = node0->w_;
        heuristics_value_ = node0->heuristics_value_;
        father_node_ = node0->father_node_;
    };
};

class Path {
public:
    vector<Node> path_;
};

class GeneratePaths {
public:
    GeneratePaths();
    ~GeneratePaths();
    vector<Path> generatePaths(double start_x, double start_y, double goal_x, double goal_y);
    vector<Path> generatePaths(double start_x, double start_y, double theta, double goal_x, double goal_y);

private:
    vector<Path> path_;
};

#endif