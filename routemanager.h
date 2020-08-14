#pragma once

#include "route.h"

#include <unordered_map>
#include <memory>
#include <iostream>
#include <map>
#include <algorithm>
#include <iterator>
#include <optional>



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
public:
	void InsertStop(Stop& stop) {
		stops_list_[stop.name] = std::move(stop);
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
		}
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
};
