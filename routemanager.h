#pragma once

#include "Route.h"

#include <unordered_map>
#include <memory>
#include <iostream>
#include <map>
#include <algorithm>
#include <iterator>


enum class TypeOfRoute {
	ANNULAR_ROUTE,
	PENDULUM_ROUTE
};

const std::map<TypeOfRoute, std::string> TYPE_SYMBOL = { {TypeOfRoute::ANNULAR_ROUTE, " > "}, 
														 {TypeOfRoute::PENDULUM_ROUTE, " - "} };

TypeOfRoute DeterminateRouteType(std::string_view stop_list) {
	char symbol;
	for (size_t i = 0; i < stop_list.size(); ++i) {
		symbol = stop_list[i];
		if (symbol == '>' || symbol == '-') {
			break;
		}
	}
	if (symbol == '>') {
		return TypeOfRoute::ANNULAR_ROUTE;
	} else if (symbol == '-') {
		return TypeOfRoute::PENDULUM_ROUTE;
	}
}

struct Bus {
	std::string name;

	TypeOfRoute type;
	std::vector<std::string> stops_list;
	std::unique_ptr<AnnularRoute> route = nullptr;
};

Bus ReadBus(std::string_view in, const std::string& name) {
	
	TypeOfRoute type = DeterminateRouteType(in);

	std::vector<std::string> result;

	while (!in.empty()) {
		result.push_back(Split(in, TYPE_SYMBOL.at(type)));
	}

	if (type == TypeOfRoute::PENDULUM_ROUTE) {
		std::vector<std::string> temp;
		std::reverse_copy(result.begin(), result.end() - 1, back_inserter(temp));
		result.insert(result.end(), make_move_iterator(temp.begin()), make_move_iterator(temp.end()));
	}
	
	return { name, type, std::move(result) };
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
		AnnularRoute::Stops stops;
		stops.reserve(bus.stops_list.size());

		for (const auto& stop_name : bus.stops_list) {
			Stop& stop = stops_list_[stop_name];
			if (stop.name.empty()) {
				stop.name = stop_name;
			}

			stop_bus_[stop_name].insert(bus.name);
			stops.push_back(&stops_list_[stop_name]);
		}

		if (bus.type == TypeOfRoute::ANNULAR_ROUTE) {
			bus.route = std::move(std::make_unique<AnnularRoute>(stops));
			bus_list_[bus.name] = std::move(bus);
		}
		else if (bus.type == TypeOfRoute::PENDULUM_ROUTE) {
			bus.route = std::move(std::make_unique<PendulumRoute>(stops));
			bus_list_[bus.name] = std::move(bus);
		}
	}

	void UpdateDb() {
		for (auto&[name, bus] : bus_list_) {
			bus.route->ComputeLenghtAndCurvature();
		}
	}

	void Bus(std::ostream& out, const std::string& bus_name) const {
		auto it = bus_list_.find(bus_name);
		if (it == bus_list_.end()) {
			out << "Bus " << bus_name << ": not found" << '\n';
		}
		else {
			out.precision(6);
			const auto& bus = it->second;
			out << "Bus " << bus.name << ": " << bus.route->CountOfStops() << " stops on route, "
				<< bus.route->CountOfUniqueStops() << " unique stops, "
				<< bus.route->GetLenght() << " route length, "
			    << bus.route->GetCurvature() << " curvature\n";
		}
	}

	void ViewStopBuses(std::ostream& out, const std::string& stop_name) const {
		auto stop = stops_list_.find(stop_name);
		auto have_bus = stop_bus_.find(stop_name);
		out << "Stop " << stop_name << ": ";
		if (stop != stops_list_.end() && have_bus != stop_bus_.end()) {
			bool first = true;
			out << "buses ";
			for (const auto& bus : stop_bus_.at(stop_name)) {
				if (first) {
					first = !first;
				}
				else {
					out << " ";
				}
				out << bus;
			}
		}
		else if (stop == stops_list_.end()) {
			out << "not found";
		}
		else {
			out << "no buses";
		}
		out << "\n";
	}
};