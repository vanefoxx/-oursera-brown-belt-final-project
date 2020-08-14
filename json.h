#pragma once

#include <istream>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace Json {

	class Node : public std::variant<std::vector<Node>,
		std::map<std::string, Node>,
		std::string,
		double,
		bool> {
	public:
		using variant::variant;

		const auto& AsArray() const {
			return std::get<std::vector<Node>>(*this);
		}
		const auto& AsMap() const {
			return std::get<std::map<std::string, Node>>(*this);
		}
		bool AsBool() const {
			return std::get<bool>(*this);
		}
		double AsDouble() const {
			return std::get<double>(*this);
		}
		const auto& AsString() const {
			return std::get<std::string>(*this);
		}
		auto& GetArray() {
			return std::get<std::vector<Node>>(*this);
		}
	};



	class Document {
	public:
		explicit Document(Node root);

		const Node& GetRoot() const;

		void AddNode(Node node) {
			root.GetArray().push_back(node);
		}

	private:
		Node root;
	};

	Document Load(std::istream& input);
}

std::ostream& operator<<(std::ostream& out, const Json::Node& node);

std::ostream& operator<<(std::ostream& out, const Json::Document& doc);
