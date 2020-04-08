/*
 * Copyright (C) 2019-2020 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/

#include "behavior_node_navigate.h"
#include "building.h"

using std::string;
using std::shared_ptr;


BehaviorNodeNavigate::BehaviorNodeNavigate(const YAML::Node& y)
: BehaviorNode()
{
  destination_name = y[1].as<string>();
}

BehaviorNodeNavigate::~BehaviorNodeNavigate()
{
}

void BehaviorNodeNavigate::print() const
{
  printf("      navigate: [%s]\n", destination_name.c_str());
}

std::unique_ptr<BehaviorNode> BehaviorNodeNavigate::clone() const
{
  return std::make_unique<BehaviorNodeNavigate>(*this);
}

void BehaviorNodeNavigate::tick(
    const double dt_seconds,
    ModelState& state,
    Building& building,
    const std::vector<std::unique_ptr<Model> >& /*active_models*/)
{
  // look up the scale of this level's drawing
  const double meters_per_pixel =
      building.level_meters_per_pixel(state.level_name);

  if (!destination_found)
  {
    destination_found = true;
    populate_model_state_from_vertex_name(
        destination_state,
        destination_name,
        building);

    const int graph_idx = 4;  // todo
    shared_ptr<planner::Graph> graph = building.planner_graph(
        graph_idx,
        state.level_name);
    graph->print();

    planner::Node goal_node;
    populate_planner_node_from_vertex_name(
        goal_node,
        destination_name,
        building);

    planner::Node start_node;
    start_node.x = state.x * meters_per_pixel;
    start_node.y = state.y * meters_per_pixel;

    graph->plan_path(start_node, goal_node);
  }


  // for now, assume just single-level navigation, no lifts, etc.

  const double error_x = destination_state.x - state.x;
  const double error_y = destination_state.y - state.y;
  const double error_2d = sqrt(error_x * error_x + error_y * error_y);

  const double bearing = atan2(error_y, error_x);

  const double speed = 0.5;  // SI units = meters/sec
  const double x_dot = speed * cos(bearing);
  const double y_dot = speed * sin(bearing);

  state.x += dt_seconds * x_dot / meters_per_pixel;
  state.y += dt_seconds * y_dot / meters_per_pixel;

  // printf("  (%.2f, %.2f)\n", state.x, state.y);

  prev_error = error_2d;
}

bool BehaviorNodeNavigate::is_complete() const
{
  return prev_error < 1.0;
}
