#include "json.h"

using namespace std;

namespace Json {

	Document::Document(Node root) : root(move(root)) {
	}

	const Node& Document::GetRoot() const {
		return root;
	}

	Node LoadNode(istream& input);

	Node LoadArray(istream& input) {
		vector<Node> result;

		for (char c; input >> c && c != ']'; ) {
			if (c != ',') {
				input.putback(c);
			}
			result.push_back(LoadNode(input));
		}

		return Node(move(result));
	}

	Node LoadDouble(istream& input) {
		double result = 0;
		input >> result;
		return Node(result);
	}

	Node LoadBool(istream& input) {
		bool result;
		input >> std::boolalpha >> result;
		return Node(result);
	}

	Node LoadString(istream& input) {
		string line;
		getline(input, line, '"');
		return Node(move(line));
	}

	Node LoadDict(istream& input) {
		map<string, Node> result;

		for (char c; input >> c && c != '}'; ) {
			if (c == ',') {
				input >> c;
			}

			string key = LoadString(input).AsString();
			input >> c;
			result.emplace(move(key), LoadNode(input));
		}

		return Node(move(result));
	}

	Node LoadNode(istream& input) {
		char c;
		input >> c;

		if (c == '[') {
			return LoadArray(input);
		}
		else if (c == '{') {
			return LoadDict(input);
		}
		else if (c == '"') {
			return LoadString(input);
		}
		else if (c == 't' || c == 'f') {
			input.putback(c);
			return LoadBool(input);
		}
		else {
			input.putback(c);
			return LoadDouble(input);
		}
	}

	Document Load(istream& input) {
		return Document{ LoadNode(input) };
	}

}

std::ostream& operator<<(std::ostream& out, const Json::Node& node) {
	if (std::holds_alternative<std::vector<Json::Node>>(node)) {
		out << "[";
		bool first = true;
		for (const auto& n : node.AsArray()) {
			if (first) {
				first = !first;
			}
			else {
				out << ", ";
			}
			out << n;
		}
		out << "]";
	}
	if (std::holds_alternative<std::map<std::string, Json::Node>>(node)) {
		out << "{";
		bool first = true;
		for (const auto&[key, value] : node.AsMap()) {
			if (first) {
				first = !first;
			}
			else {
				out << ", ";
			}
			out << '"' << key << '"' << ": " << value;
		}
		out << "}";
	}
	if (std::holds_alternative<bool>(node)) {
		out << std::boolalpha << node.AsBool();
	}
	if (std::holds_alternative<std::string>(node)) {
		out << '"' << node.AsString() << '"';
	}
	if (std::holds_alternative<double>(node)) {
		double num = node.AsDouble();
		int int_part = static_cast<int> (num);
		if (num == int_part) {
			out << int_part;
		}
		else {
			out << num;
		}
	}

	return out;
}

std::ostream& operator<<(std::ostream& out, const Json::Document& doc) {
	return out << doc.GetRoot();
}