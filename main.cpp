#include "route.h"
#include "routemanager.h"
#include "json.h"

#include <iostream>
#include <optional>
#include <sstream>
#include <iomanip>


using namespace std;

class Request {
public:
	Request(RouteManager& rm_ref) : rm_ref_(rm_ref) {}

	virtual void Run() = 0;
protected:

	RouteManager& rm_ref_;
};

class BaseRequest : public Request {
public:
	BaseRequest(RouteManager& rm_ref) : Request(rm_ref) {}
};

class InsertStopRequest : public BaseRequest {
public:
	InsertStopRequest(RouteManager& rm_ref, Stop stop) : BaseRequest(rm_ref),
		stop_(move(stop)) {}

	void Run() override {
		rm_ref_.InsertStop(stop_);
	}
private:
	Stop stop_;
};

class InsertBusRequest : public BaseRequest {
public:
	InsertBusRequest(RouteManager& rm_ref, Bus bus) : BaseRequest(rm_ref),
		bus_(move(bus)) {}

	void Run() override {
		rm_ref_.InsertBus(bus_);
	}
private:
	Bus bus_;
};

class StatsRequest : public Request {
public:
	StatsRequest(RouteManager& rm_ref, int id, Json::Document& out, string search_name = "") : Request(rm_ref),
		id_(id),
		out_(out),
		search_name_(search_name) {}
protected:
	string search_name_;

	int id_;
	Json::Document& out_;
};

class BusInfoRequest : public StatsRequest {
public:
	BusInfoRequest(RouteManager& rm_ref, int id, Json::Document& out, string search_name = "") : StatsRequest(rm_ref, id, out, search_name) {}

	void Run() override {
		rm_ref_.Bus(out_, id_, search_name_);
	}
};

class StopInfoRequest : public  StatsRequest {
public:
	StopInfoRequest(RouteManager& rm_ref, int id, Json::Document& out, string search_name = "") : StatsRequest(rm_ref, id, out, search_name) {}

	void Run() override {
		rm_ref_.ViewStopBuses(out_, id_, search_name_);
	}
};

class UpdateQuery : public BaseRequest {
public:
	UpdateQuery(RouteManager& rm_ref) : BaseRequest(rm_ref) {}

	void Run() override {
		rm_ref_.UpdateDb();
	}
};

vector<unique_ptr<Request>> ReadQueries(const Json::Document& in, Json::Document& out, RouteManager& rm) {

	const auto& requests = in.GetRoot().AsMap();
	const auto& base_requests = requests.at("base_requests").AsArray();

	size_t count_of_update_queries = base_requests.size();

	vector <unique_ptr<Request>> result;
	result.reserve(count_of_update_queries);

	for (size_t i = 0; i < count_of_update_queries; ++i) {
		const auto& request = base_requests[i].AsMap();
		const auto& type = request.at("type").AsString();

		if (type == "Stop") {
			result.push_back(make_unique<InsertStopRequest>(rm, ReadStop(request)));
		}
		else if (type == "Bus") {
			result.push_back(make_unique<InsertBusRequest>(rm, ReadBus(request)));
		}
	}

	result.push_back(make_unique<UpdateQuery>(rm));

	const auto& stat_requests = requests.at("stat_requests").AsArray();

	size_t count_of_read_queries = stat_requests.size();

	result.reserve(count_of_update_queries + count_of_read_queries);

	for (size_t i = 0; i < count_of_read_queries; ++i) {
		const auto& request = stat_requests[i].AsMap();
		const auto& type = request.at("type").AsString();
		const auto& name = request.at("name").AsString();
		int id = request.at("id").AsDouble();

		if (type == "Bus") {
			result.push_back(make_unique<BusInfoRequest>(rm, id, out, name));
		}
		else if (type == "Stop") {
			result.push_back(make_unique<StopInfoRequest>(rm, id, out, name));
		}
	}

	return result;
}

void Run(istream& in, ostream& out) {
	Json::Document in_doc = Json::Load(in);

	vector<Json::Node> root;
	Json::Document out_doc(root);

	RouteManager rm;
	auto queries = ReadQueries(in_doc, out_doc, rm);
	for (auto& query : queries) {
		query->Run();
	}
	out << setprecision(6);
	out << out_doc;
}

int main() {/*
	istringstream in(R"({
  "base_requests": [
	{
	  "type": "Stop",
	  "road_distances": {
		"Marushkino": 3900
	  },
	  "longitude": 37.20829,
	  "name": "Tolstopaltsevo",
	  "latitude": 55.611087
	},
	{
	  "type": "Stop",
	  "road_distances": {
		"Rasskazovka": 9900
	  },
	  "longitude": 37.209755,
	  "name": "Marushkino",
	  "latitude": 55.595884
	},
	{
	  "type": "Bus",
	  "name": "256",
	  "stops": [
		"Biryulyovo Zapadnoye",
		"Biryusinka",
		"Universam",
		"Biryulyovo Tovarnaya",
		"Biryulyovo Passazhirskaya",
		"Biryulyovo Zapadnoye"
	  ],
	  "is_roundtrip": true
	},
	{
	  "type": "Bus",
	  "name": "750",
	  "stops": [
		"Tolstopaltsevo",
		"Marushkino",
		"Rasskazovka"
	  ],
	  "is_roundtrip": false
	},
	{
	  "type": "Stop",
	  "road_distances": {},
	  "longitude": 37.333324,
	  "name": "Rasskazovka",
	  "latitude": 55.632761
	},
	{
	  "type": "Stop",
	  "road_distances": {
		"Rossoshanskaya ulitsa": 7500,
		"Biryusinka": 1800,
		"Universam": 2400
	  },
	  "longitude": 37.6517,
	  "name": "Biryulyovo Zapadnoye",
	  "latitude": 55.574371
	},
	{
	  "type": "Stop",
	  "road_distances": {
		"Universam": 750
	  },
	  "longitude": 37.64839,
	  "name": "Biryusinka",
	  "latitude": 55.581065
	},
	{
	  "type": "Stop",
	  "road_distances": {
		"Rossoshanskaya ulitsa": 5600,
		"Biryulyovo Tovarnaya": 900
	  },
	  "longitude": 37.645687,
	  "name": "Universam",
	  "latitude": 55.587655
	},
	{
	  "type": "Stop",
	  "road_distances": {
		"Biryulyovo Passazhirskaya": 1300
	  },
	  "longitude": 37.653656,
	  "name": "Biryulyovo Tovarnaya",
	  "latitude": 55.592028
	},
	{
	  "type": "Stop",
	  "road_distances": {
		"Biryulyovo Zapadnoye": 1200
	  },
	  "longitude": 37.659164,
	  "name": "Biryulyovo Passazhirskaya",
	  "latitude": 55.580999
	},
	{
	  "type": "Bus",
	  "name": "828",
	  "stops": [
		"Biryulyovo Zapadnoye",
		"Universam",
		"Rossoshanskaya ulitsa",
		"Biryulyovo Zapadnoye"
	  ],
	  "is_roundtrip": true
	},
	{
	  "type": "Stop",
	  "road_distances": {},
	  "longitude": 37.605757,
	  "name": "Rossoshanskaya ulitsa",
	  "latitude": 55.595579
	},
	{
	  "type": "Stop",
	  "road_distances": {},
	  "longitude": 37.603831,
	  "name": "Prazhskaya",
	  "latitude": 55.611678
	}
  ],
  "stat_requests": [
	{
	  "type": "Bus",
	  "name": "256",
	  "id": 1965312327
	},
	{
	  "type": "Bus",
	  "name": "750",
	  "id": 519139350
	},
	{
	  "type": "Bus",
	  "name": "751",
	  "id": 194217464
	},
	{
	  "type": "Stop",
	  "name": "Samara",
	  "id": 746888088
	},
	{
	  "type": "Stop",
	  "name": "Prazhskaya",
	  "id": 65100610
	},
	{
	  "type": "Stop",
	  "name": "Biryulyovo Zapadnoye",
	  "id": 1042838872
	}
  ]
})");*/


	Run(cin, cout);
	//system("pause");
	return 0;
}