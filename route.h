#pragma once
#include "json.h"

#include <string_view>
#include <string>
#include <vector>
#include <set>
#include <cmath>
#include <unordered_map>
#include <iostream>
#include <sstream>
#include <cstdint>
#include <map>


struct Stop {
	std::string name;
	double latitude;
	double longitude;

	using StopsDistance = std::unordered_map<std::string, int>;
	StopsDistance other_stops_distance;
};

template<typename T>
T ConvertInNumber(const std::string& str) {
	std::istringstream to_double(str);
	T result;
	to_double >> result;
	return result;
}

std::string Split(std::string_view& sv, const std::string& symbols) {
	size_t pos = sv.find(symbols);
	std::string result(sv.substr(0, pos));
	if (pos != sv.npos) {
		sv.remove_prefix(pos + symbols.size());
	}
	else {
		sv.remove_prefix(sv.size());
	}
	return result;
}

Stop ReadStop(const std::map<std::string, Json::Node>& json_stop) {
	Stop stop;

	stop.name = json_stop.at("name").AsString();
	stop.latitude = json_stop.at("latitude").AsDouble();
	stop.longitude = json_stop.at("longitude").AsDouble();

	const auto& road_distances = json_stop.at("road_distances").AsMap();

	for (const auto&[stop_name, distance_node] : road_distances) {
		stop.other_stops_distance[stop_name] = distance_node.AsDouble();
	}

	return stop;
}

int ComputeRouteDistance(const Stop& first, const Stop& second) {
	const auto& stops_distance_first = first.other_stops_distance;
	const auto& stops_distance_second = second.other_stops_distance;

	return stops_distance_first.find(second.name) == stops_distance_first.end()
		? stops_distance_second.at(first.name)
		: stops_distance_first.at(second.name);
}

class Route {
public:
	using Stops = std::vector<const Stop*>;
protected:

	Stops stops_;
	std::set<std::string_view> unique_stops_;
	int route_lenght_ = 0;
	double curvature_ = 0;

	static std::pair<double, double> ConvertCoordToRadian(const Stop& stop) {
		static const double PI = 3.1415926535;

		return { stop.latitude * PI / 180.0, stop.longitude * PI / 180.0 };
	}

	static double ComputeGeoDistance(const Stop& first, const Stop& second) {
		static const uint32_t EARTH_RADIUS = 6'371'000;

		auto first_in_rad = ConvertCoordToRadian(first);
		auto second_in_rad = ConvertCoordToRadian(second);

		double angle = acos(sin(first_in_rad.first) * sin(second_in_rad.first) +
			cos(first_in_rad.first) * cos(second_in_rad.first) * cos(first_in_rad.second - second_in_rad.second));

		return angle * EARTH_RADIUS;
	}

public:
	
	Route(Stops& stops) : stops_(std::move(stops)) {
		for (auto stop : stops_) {
			unique_stops_.insert(stop->name);
		}
	}

	void ComputeLenghtAndCurvature() {
		double route_geolenght = 0;

		for (size_t i = 0; i < stops_.size() - 1; ++i) {
			route_lenght_ += ComputeRouteDistance(*stops_[i], *stops_[i + 1]);

			route_geolenght += ComputeGeoDistance(*stops_[i], *stops_[i + 1]);
		}

		curvature_ = route_lenght_ / route_geolenght;
	}

	size_t CountOfStops() const {
		return stops_.size();
	}

	size_t CountOfUniqueStops() const {
		return unique_stops_.size();
	}

	int GetLenght() const {
		return route_lenght_;
	}

	double GetCurvature() const {
		return curvature_;
	}
};
