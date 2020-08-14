#pragma once

#include "route.h"
#include "graph.h"
#include "router.h"

#include <unordered_map>
#include <memory>
#include <iostream>
#include <map>
#include <algorithm>
#include <iterator>
#include <optional>

enum class ActivityType {
	BUS,
	WAIT
};

struct Activity {
	ActivityType type;
	std::string name;
	int count = 0;
	double time = 0;

	Activity(ActivityType t, const std::string& str, int c, double ti) : type(t), name(str), count(c), time(ti) {}
	Activity(int ti) : time(ti) {}
	Activity() {};
};

bool operator>=(const Activity& left, int right) {
	return left.time >= right;
}

bool operator<(const Activity& left, const Activity& right) {
	return left.time < right.time;
}

bool operator>(const Activity& left, const Activity& right) {
	return left.time > right.time;
}

Activity operator+(const Activity& left, const Activity& right) {
	Activity result;
	result.count = left.count + right.count;
	result.time = left.time + right.time;
	return result;
}

struct Bus {
	std::string name;

	bool is_roundtrip;
	std::vector<std::string> stops_list;
	std::optional<Route> route = std::nullopt;

};

Bus ReadBus(const std::map<std::string, Json::Node>& json_bus) {
	Bus bus;

	bus.name = json_bus.at("name").AsString();

	bus.is_roundtrip = json_bus.at("is_roundtrip").AsBool();

	for (const auto& stop_name_node : json_bus.at("stops").AsArray()) {
		bus.stops_list.push_back(stop_name_node.AsString());
	}

	if (!bus.is_roundtrip) {
		std::vector<std::string> temp;
		std::reverse_copy(bus.stops_list.begin(), bus.stops_list.end() - 1, back_inserter(temp));
		bus.stops_list.insert(bus.stops_list.end(), make_move_iterator(temp.begin()), make_move_iterator(temp.end()));
	}

	return bus;
}

class RouteManager {
private:
	std::unordered_map<std::string, Stop> stops_list_;
	std::unordered_map<std::string, Bus> bus_list_;
	std::unordered_map<std::string, std::set<std::string>> stop_bus_;

	int bus_wait_time = 0;
	double bus_velocity = 0;

	Graph::DirectedWeightedGraph<Activity> graph_;
	std::optional<Graph::Router<Activity>> router_ = std::nullopt;
	
	Graph::VertexId next_vertex = 0;

	std::unordered_map<std::string, Graph::VertexId> stop_graph_pos;

	void BuildRouteInGraph(const Bus& bus) {
		const auto& stops_list = bus.stops_list;
		for (size_t i = 0; i < stops_list.size(); ++i) {
			
			const auto current_stop_pos = stop_graph_pos.at(stops_list[i]) + 1;
			
			double distance = 0;
			for (size_t j = i + 1; j < stops_list.size(); ++j) {
				
				if (stops_list[i] != stops_list[j]) {
					distance += ComputeRouteDistance(stops_list_.at(stops_list[j - 1]), stops_list_.at(stops_list[j]));
					graph_.AddEdge({ current_stop_pos,
									 stop_graph_pos.at(stops_list[j]),
									 { ActivityType::BUS, bus.name, static_cast<int> (j - i), distance / bus_velocity * 60 / 1000 } });
				}
			}
		}
	
	}

public:
	RouteManager() : graph_(200) {}

	void InsertStop(Stop& stop) {
		
		std::string name = stop.name;
		stop_graph_pos[stop.name] = next_vertex;
		stops_list_[stop.name] = std::move(stop);
		
		graph_.AddEdge({ next_vertex, 
						 next_vertex + 1, 
			             { ActivityType::WAIT, move(name), 0, static_cast<double> (bus_wait_time) }});
		next_vertex += 2;
	}

	void GetRoutesSettings(int wait_time, double velocity) {
		bus_wait_time = wait_time;
		bus_velocity = velocity;
	}

	void InsertBus(Bus& bus) {
		Route::Stops stops;
		stops.reserve(bus.stops_list.size());

		for (const auto& stop_name : bus.stops_list) {
			Stop& stop = stops_list_[stop_name];
			if (stop.name.empty()) {
				stop.name = stop_name;
			}

			stop_bus_[stop_name].insert(bus.name);
			stops.push_back(&stops_list_[stop_name]);
		}

		bus.route = std::make_optional <Route>(stops);
		bus_list_[bus.name] = std::move(bus);
	}

	void UpdateDb() {
		
		for (auto&[name, bus] : bus_list_) {
			bus.route->ComputeLenghtAndCurvature();
			BuildRouteInGraph(bus);
		}
		
		router_.emplace(graph_);
	}

	void Bus(Json::Document& out, int id, const std::string& bus_name) const {
		auto it = bus_list_.find(bus_name);
		std::map<std::string, Json::Node> node;
		if (it == bus_list_.end()) {
			node.emplace("request_id", Json::Node(static_cast<double> (id)));
			node.emplace("error_message", Json::Node(std::string("not found")));
		}
		else {
			const auto& bus = it->second;

			node.emplace("stop_count", Json::Node(static_cast<double> (bus.route->CountOfStops())));
			node.emplace("unique_stop_count", Json::Node(static_cast<double> (bus.route->CountOfUniqueStops())));
			node.emplace("route_length", Json::Node(static_cast<double> (bus.route->GetLenght())));
			node.emplace("curvature", Json::Node(bus.route->GetCurvature()));
			node.emplace("request_id", Json::Node(static_cast<double> (id)));
		}

		out.AddNode(Json::Node(move(node)));
	}

	void ViewStopBuses(Json::Document& out, int id, const std::string& stop_name) const {
		auto stop = stops_list_.find(stop_name);
		auto have_bus = stop_bus_.find(stop_name);
		std::map<std::string, Json::Node> node;
		std::vector<Json::Node> buses;
		if (stop != stops_list_.end() && have_bus != stop_bus_.end()) {
			for (const auto& bus : stop_bus_.at(stop_name)) {
				buses.push_back(Json::Node(std::string(bus)));
			}
			node.emplace("buses", Json::Node(move(buses)));
		}
		else if (stop == stops_list_.end()) {
			node.emplace("error_message", Json::Node(std::string("not found")));
		}
		else {
			node.emplace("buses", Json::Node(move(buses)));
		}
		node.emplace("request_id", Json::Node(static_cast<double>(id)));

		out.AddNode(Json::Node(move(node)));
	}

	void BuildRoute(Json::Document& out, int id, const std::string& from, const std::string& to) const {
		const auto& route = router_->BuildRoute(stop_graph_pos.at(from), stop_graph_pos.at(to));
		std::map<std::string, Json::Node> node;
		std::vector<Json::Node> items;
		node.emplace("request_id", Json::Node(static_cast<double>(id)));
		if (route != std::nullopt) {
			for (size_t i = 0; i < route->edge_count; ++i) {
				const auto& ref = graph_.GetEdge(router_->GetRouteEdge(route->id, i)).weight;
				std::map<std::string, Json::Node> act;
				if (ref.type == ActivityType::BUS) {
					act.emplace("time", Json::Node(ref.time));
					act.emplace("type", Json::Node(std::string("Bus")));
					act.emplace("bus", Json::Node(ref.name));
					act.emplace("span_count", Json::Node(static_cast<double>(ref.count)));
				}
				else if (ref.type == ActivityType::WAIT) {
					act.emplace("time", Json::Node(ref.time));
					act.emplace("type", Json::Node(std::string("Wait")));
					act.emplace("stop_name", Json::Node(ref.name));
				}

				items.push_back(Json::Node(move(act)));
			}


			node.emplace("items", Json::Node(move(items)));
			node.emplace("total_time", Json::Node(route->weight.time));
		} else {
			node.emplace("error_message", Json::Node(std::string("not found")));
		}

		out.AddNode(Json::Node(move(node)));
	}
};
