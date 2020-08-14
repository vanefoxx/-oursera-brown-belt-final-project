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

class GetSettingsRequest : public BaseRequest {
public:
	GetSettingsRequest(RouteManager& rm_ref, int time, double velocity) : BaseRequest(rm_ref),
		time_(time), velocity_(velocity){}

	void Run() override {
		rm_ref_.GetRoutesSettings(time_, velocity_);
	}
private:
	int time_;
	double velocity_;
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

class BuildRouteRequest : public  StatsRequest {
public:
	BuildRouteRequest(RouteManager& rm_ref, int id, Json::Document& out, string from, string to) : StatsRequest(rm_ref, id, out), from_(from), to_(to) {}

	void Run() override {
		rm_ref_.BuildRoute(out_, id_, from_, to_);
	}
private:
	string from_;
	string to_;
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
	const auto& routing_settings = requests.at("routing_settings").AsMap();
	const auto& base_requests = requests.at("base_requests").AsArray();

	size_t count_of_update_queries = base_requests.size();

	vector <unique_ptr<Request>> result;
	result.reserve(count_of_update_queries);

	result.push_back(make_unique<GetSettingsRequest>(rm, routing_settings.at("bus_wait_time").AsDouble(),
		routing_settings.at("bus_velocity").AsDouble()));

	for (size_t i = 0; i < count_of_update_queries; ++i) {
		const auto& request = base_requests[i].AsMap();
		
		const auto& type = request.at("type").AsString();
		
		if (type == "Stop") {
			result.push_back(make_unique<InsertStopRequest>(rm, ReadStop(request)));
		} else if (type == "Bus") {
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
		int id = request.at("id").AsDouble();
		
		if (type == "Bus") {
			const auto& name = request.at("name").AsString();
			result.push_back(make_unique<BusInfoRequest>(rm, id, out, name));
		} else if (type == "Stop") {
			const auto& name = request.at("name").AsString();
			result.push_back(make_unique<StopInfoRequest>(rm, id, out, name));
		} else if (type == "Route") {
			result.push_back(make_unique<BuildRouteRequest>(rm, id, out, request.at("from").AsString(), 
																		 request.at("to").AsString()));
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

int main() {
	
	Run(cin, cout);
	
	return 0;
}