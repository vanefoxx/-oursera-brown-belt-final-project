#pragma once

#include "Route.h"

#include <unordered_map>
#include <memory>
#include <iostream>

enum class TypeOfRoute {
	ANNULAR_ROUTE,
	PENDULUM_ROUTE
};

std::pair<TypeOfRoute, std::vector<std::string>> ReadStops(std::istream& in) {
	std::string s;

	std::getline(in, s);

	char symbol;
	TypeOfRoute type;

	if (size_t pos = s.find('-'); pos != s.npos) {
		type = TypeOfRoute::PENDULUM_ROUTE;
		symbol = '-';
	} else if (size_t pos = s.find('>'); pos != s.npos) {
		type = TypeOfRoute::ANNULAR_ROUTE;
		symbol = '>';
	}

	std::vector<std::string> result;

	std::string_view s_v(s);

	for (size_t pos; !s_v.empty(); ) {
		pos = s_v.find(symbol);
		result.push_back(std::string(s_v.substr(0, pos != s_v.npos ? pos - 1 : s_v.npos)));
		s_v.remove_prefix(pos != s.npos ? pos + 2 : s_v.size());
	}

	return { type, std::move(result) };
}

class RouteManager {
private:
	std::unordered_map<std::string, Stop> stops_list_;
	std::unordered_map<std::string, std::unique_ptr<AnnularRoute>> bus_list_;
	std::unordered_map<std::string, std::set<std::string>> stop_bus_;
public:
	void InsertStop(Stop stop) {
		stops_list_[stop.name] = stop;
	}

	void InsertRoute(const std::string& route_name, TypeOfRoute type, const std::vector<std::string>& names_of_stops) {
		Stops stops;
		stops.reserve(names_of_stops.size());

		for (const auto& stop_name : names_of_stops) {
			Stop& stop = stops_list_[stop_name];
			if (stop.name.empty()) {
				stop.name = stop_name;
			}

			stop_bus_[stop_name].insert(route_name);
			stops.push_back(&stops_list_[stop_name]);
		}

		if (type == TypeOfRoute::ANNULAR_ROUTE) {
			bus_list_[route_name] = std::make_unique<AnnularRoute>(stops);
		}
		else if (type == TypeOfRoute::PENDULUM_ROUTE) {
			bus_list_[route_name] = std::make_unique<PendulumRoute>(stops);
		}
	}

	void UpdateDb() {
		for (auto& [name, route] : bus_list_) {
			route->ComputeLenght();
		}
	}

	void Bus(std::ostream& out, const std::string& bus_name) const {
		auto it = bus_list_.find(bus_name);
		if (it == bus_list_.end()) {
			out << "Bus " << bus_name << ": not found" << '\n';
		} else {
			out.precision(6);
			out << "Bus " << bus_name << ": " << (it->second)->CountOfStops() << " stops on route, "
				                             << (it->second)->CountOfUniqueStops() << " unique stops, "
				                             << (it->second)->GetLenght() << " route length\n";
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
				} else {
					out << " ";
				}
				out << bus;
			}
		} else if (stop == stops_list_.end()) {
			out << "not found";
		} else {
			out << "no buses";
		}
		out << "\n";
	}
};