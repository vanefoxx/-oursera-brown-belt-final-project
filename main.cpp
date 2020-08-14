#include "Route.h"
#include "RouteManager.h"

#include <iostream>
#include <optional>
#include <sstream>
#include <iomanip>

using namespace std;

class Query {
public:
	Query(RouteManager& rm_ref, const string& name = "") : rm_ref_(rm_ref), name_(name) {}
	
	virtual void Run() = 0;
protected:
	RouteManager& rm_ref_;

	string name_;
};

class InsertStopQuery : public Query {
public:
	InsertStopQuery(RouteManager& rm_ref, const string& name, double latitude, double longitude) : Query(rm_ref, name),
																			 latitude_(latitude), longitude_(longitude) {}
	void Run() override {
		rm_ref_.InsertStop(Stop{ name_, latitude_, longitude_ });
	}
private:
	double latitude_;
	double longitude_;
};

class InsertRouteQuery : public Query {
public:
	InsertRouteQuery(RouteManager& rm_ref, const string& name, TypeOfRoute type, vector<string>& names) : Query(rm_ref, name),
		type_of_route(type), names_of_stops(move(names)) {}
	void Run() override {
		rm_ref_.InsertRoute(name_, type_of_route, names_of_stops);
	}
private:
	TypeOfRoute type_of_route;
	vector<string> names_of_stops;
};

class ReadRouteQuery : public Query {
public:
	ReadRouteQuery(RouteManager& rm_ref, const string& name, ostream& out) : Query(rm_ref, name), out_(out) {}

	void Run() override {
		rm_ref_.Bus(out_, name_);
	}
private:
	ostream& out_;
};

class ReadStopQuery : public Query {
public:
	ReadStopQuery(RouteManager& rm_ref, const string& name, ostream& out) : Query(rm_ref, name), out_(out) {}

	void Run() override {
		rm_ref_.ViewStopBuses(out_, name_);
	}
private:
	ostream& out_;
};

class UpdateQuery : public Query {
public:
	UpdateQuery(RouteManager& rm_ref) : Query(rm_ref) {}

	void Run() override {
		rm_ref_.UpdateDb();
	}
};

vector<unique_ptr<Query>> ReadQueries(istream& in, ostream& out, RouteManager& rm) {
	size_t count_of_update_queries = 0;
	in >> count_of_update_queries;

	vector <unique_ptr<Query>> result;
	result.reserve(count_of_update_queries);

	for (size_t i = 0; i < count_of_update_queries; ++i) {
		string query;
		in >> query;
		in.ignore(1);

		string name;
		getline(in, name, ':');
		in.ignore(1);

		if (query == "Stop") {
			double latitude, longitude;
			in >> latitude;
			in.ignore(2);
			in >> longitude;

			result.push_back(make_unique<InsertStopQuery>(rm, name, latitude, longitude ));
		}
		else if (query == "Bus") {
			auto[type, names] = ReadStops(in);
			result.push_back(make_unique<InsertRouteQuery>(rm, name, type, names));
		}
	}

	result.push_back(make_unique<UpdateQuery>(rm));

	size_t count_of_read_queries;
	in >> count_of_read_queries;

	result.reserve(count_of_update_queries + count_of_read_queries);

	for (size_t i = 0; i < count_of_read_queries; ++i) {
		string query;
		in >> query;
		in.ignore(1);

		string name;
		getline(in, name);

		if (query == "Bus") {
			result.push_back(make_unique<ReadRouteQuery>(rm, name, out));
		} else if (query == "Stop") {
			result.push_back(make_unique<ReadStopQuery>(rm, name, out));
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

	system("pause");
	return 0;
}