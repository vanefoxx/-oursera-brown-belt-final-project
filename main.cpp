#include "Route.h"
#include "RouteManager.h"

#include <iostream>
#include <optional>
#include <sstream>
#include <iomanip>

using namespace std;

class Query {
public:
	Query(RouteManager& rm_ref, const string& name = "", ostream& out = cout) : rm_ref_(rm_ref), name_(name), out_(out) {}

	virtual void Run() = 0;
protected:

	RouteManager& rm_ref_;
	string name_;
	ostream& out_;
};

class InsertStopQuery : public Query {
public:
	InsertStopQuery(RouteManager& rm_ref, Stop stop) : Query(rm_ref),
		stop_(move(stop)) {}

	void Run() override {
		rm_ref_.InsertStop(stop_);
	}
private:
	Stop stop_;
};

class InsertBusQuery : public Query {
public:
	InsertBusQuery(RouteManager& rm_ref, Bus bus) : Query(rm_ref),
		bus_(move(bus)) {}

	void Run() override {
		rm_ref_.InsertBus(bus_);
	}
private:
	Bus bus_;
};

class BusInfoQuery : public Query {
public:
	BusInfoQuery(RouteManager& rm_ref, const string& name, ostream& out = cout) : Query(rm_ref, name, out) {}

	void Run() override {
		rm_ref_.Bus(out_, name_);
	}
};

class StopInfoQuery : public Query {
public:
	StopInfoQuery(RouteManager& rm_ref, const string& name, ostream& out = cout) : Query(rm_ref, name, out) {}

	void Run() override {
		rm_ref_.ViewStopBuses(out_, name_);
	}
};

class UpdateQuery : public Query {
public:
	UpdateQuery(RouteManager& rm_ref) : Query(rm_ref) {}

	void Run() override {
		rm_ref_.UpdateDb();
	}
};

vector<unique_ptr<Query>> ReadQueries(istream& in, ostream& out, RouteManager& rm) {

	string queries;
	
	getline(in, queries, 'é');

	string_view sv(queries);
	
	size_t count_of_update_queries = ConvertInNumber<size_t>(Split(sv, "\n"));
	
	vector <unique_ptr<Query>> result;
	result.reserve(count_of_update_queries);

	for (size_t i = 0; i < count_of_update_queries; ++i) {
		string query = Split(sv, " ");
		
		string name = Split(sv, ": ");
		
		if (query == "Stop") {
			result.push_back(make_unique<InsertStopQuery>(rm, ReadStop(Split(sv, "\n"), name)));
		}
		else if (query == "Bus") {
			result.push_back(make_unique<InsertBusQuery>(rm, ReadBus(Split(sv, "\n"), name)));
		}
	}
	
	result.push_back(make_unique<UpdateQuery>(rm));

	size_t count_of_read_queries = ConvertInNumber<size_t>(Split(sv, "\n"));
	
	result.reserve(count_of_update_queries + count_of_read_queries);
	
	for (size_t i = 0; i < count_of_read_queries; ++i) {
		string query = Split(sv, " ");
		
		string name = Split(sv, "\n");

		if (query == "Bus") {
			result.push_back(make_unique<BusInfoQuery>(rm, name, out));
		}
		else if (query == "Stop") {
			result.push_back(make_unique<StopInfoQuery>(rm, name, out));
		}
	}

	return result;
}

void Run(istream& in, ostream& out) {
	RouteManager rm;
	auto queries = ReadQueries(in, out, rm);
	for (auto& query : queries) {
		query->Run();
	}
}

int main() {
	
	Run(cin, cout);

	return 0;
}