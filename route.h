#pragma once

#include <string_view>
#include <string>
#include <vector>
#include <set>
#include <cmath>

struct Stop {
	std::string name;
	double latitude;
	double longitude;
};

using Stops = std::vector<const Stop*>;

class AnnularRoute {
protected:

	Stops stops_;
	std::set<std::string_view> unique_stops_;
	double route_lenght_;

	static std::pair<double, double> ConvertCoordToRadian(const Stop& stop) {
		static const double PI = 3.1415926535;

		return { stop.latitude * PI / 180.0, stop.longitude * PI / 180.0 };
	}

	static double ComputeDistance(const Stop& first, const Stop& second) {
		static const uint32_t EARTH_RADIUS = 6'371'000;

		auto first_in_rad = ConvertCoordToRadian(first);
		auto second_in_rad = ConvertCoordToRadian(second);

		double angle = acos(sin(first_in_rad.first) * sin(second_in_rad.first) +
			cos(first_in_rad.first) * cos(second_in_rad.first) * cos(first_in_rad.second - second_in_rad.second));

		return angle * EARTH_RADIUS;
	}

public:
	AnnularRoute(Stops& stops) : stops_(std::move(stops)) {
		for (auto stop : stops_) {
			unique_stops_.insert(stop->name);
		}
	}

	void ComputeLenght() {
		double result = 0;
		for (size_t i = 0; i < stops_.size() - 1; ++i) {
			result += ComputeDistance(*stops_[i], *stops_[i + 1]);
		}

		route_lenght_ = result;
	}

	virtual size_t CountOfStops() const{
		return stops_.size();
	}

	size_t CountOfUniqueStops() const {
		return unique_stops_.size();
	}

	virtual double GetLenght() const {
		return route_lenght_;
	}

	virtual ~AnnularRoute() {}
};

class PendulumRoute : public AnnularRoute {
public:
	PendulumRoute(Stops& stops) : AnnularRoute(stops) {}

	size_t CountOfStops() const override{
		return 2 * stops_.size() - 1;
	}

	double GetLenght() const override {
		return 2 * route_lenght_;
	}
};