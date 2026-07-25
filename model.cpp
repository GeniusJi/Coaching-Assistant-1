#include <bits/stdc++.h>
using namespace std;

const int MAXN = 1000;
const int FEATURES = 10;
const int TREES = 200;
const int MAX_DEPTH = 10;
const int MIN_LEAF = 3;

struct Player {
	string name;
	double stat[FEATURES];
	int position;
};

struct Node {
	int feature;
	int position;
	double split;
	Node *left;
	Node *right;
};

Player players[MAXN];
Player trainData[MAXN];
Player testData[MAXN];

Node *forest[TREES];

int playerCount;
int trainCount;
int testCount;

mt19937 rng(42);

vector<string> splitCSV(string line) {
	vector<string> result;
	string current = "";
	bool quotes = false;

	for (char c : line) {
		if (c == '"') {
			quotes = !quotes;
		} else if (c == ',' && !quotes) {
			result.push_back(current);
			current = "";
		} else {
			current += c;
		}
	}

	result.push_back(current);
	return result;
}

int getPosition(string position) {
	if (position == "DEF") return 0;
	if (position == "MID") return 1;
	return 2;
}

string getPositionName(int position) {
	if (position == 0) return "DEF";
	if (position == 1) return "MID";
	return "FWD";
}

double getGini(int count[3], int total) {
	if (total == 0) return 0;

	double gini = 1;

	for (int i = 0; i < 3; i++) {
		double probability = (double)count[i] / total;
		gini -= probability * probability;
	}

	return gini;
}

int getMajority(int ids[], int size) {
	int count[3] = {};

	for (int i = 0; i < size; i++) {
		count[trainData[ids[i]].position]++;
	}

	int answer = 0;

	for (int i = 1; i < 3; i++) {
		if (count[i] > count[answer]) {
			answer = i;
		}
	}

	return answer;
}

Node *buildTree(int ids[], int size, int depth) {
	Node *node = new Node();

	node->feature = -1;
	node->position = getMajority(ids, size);
	node->left = NULL;
	node->right = NULL;

	bool same = true;

	for (int i = 1; i < size; i++) {
		if (trainData[ids[i]].position != trainData[ids[0]].position) {
			same = false;
		}
	}

	if (same || depth == MAX_DEPTH || size < MIN_LEAF * 2) {
		return node;
	}

	int featureOrder[FEATURES];

	for (int i = 0; i < FEATURES; i++) {
		featureOrder[i] = i;
	}

	shuffle(featureOrder, featureOrder + FEATURES, rng);

	int featuresChecked = sqrt(FEATURES);

	int bestFeature = -1;
	double bestSplit = 0;
	double bestScore = 1e18;

	for (int z = 0; z < featuresChecked; z++) {
		int feature = featureOrder[z];

		vector<pair<double, int>> values;

		for (int i = 0; i < size; i++) {
			int id = ids[i];

			values.push_back({
				trainData[id].stat[feature],
				trainData[id].position
			});
		}

		sort(values.begin(), values.end());

		int leftCount[3] = {};
		int rightCount[3] = {};

		for (int i = 0; i < size; i++) {
			rightCount[values[i].second]++;
		}

		for (int i = 0; i < size - 1; i++) {
			int position = values[i].second;

			leftCount[position]++;
			rightCount[position]--;

			int leftSize = i + 1;
			int rightSize = size - leftSize;

			if (leftSize < MIN_LEAF || rightSize < MIN_LEAF) {
				continue;
			}

			if (values[i].first == values[i + 1].first) {
				continue;
			}

			double score =
				leftSize * getGini(leftCount, leftSize) +
				rightSize * getGini(rightCount, rightSize);

			if (score < bestScore) {
				bestScore = score;
				bestFeature = feature;
				bestSplit =
					(values[i].first + values[i + 1].first) / 2;
			}
		}
	}

	if (bestFeature == -1) {
		return node;
	}

	int leftIds[MAXN];
	int rightIds[MAXN];

	int leftSize = 0;
	int rightSize = 0;

	for (int i = 0; i < size; i++) {
		int id = ids[i];

		if (trainData[id].stat[bestFeature] <= bestSplit) {
			leftIds[leftSize++] = id;
		} else {
			rightIds[rightSize++] = id;
		}
	}

	node->feature = bestFeature;
	node->split = bestSplit;

	node->left = buildTree(leftIds, leftSize, depth + 1);
	node->right = buildTree(rightIds, rightSize, depth + 1);

	return node;
}

int predict(Node *node, Player player) {
	while (node->feature != -1) {
		if (player.stat[node->feature] <= node->split) {
			node = node->left;
		} else {
			node = node->right;
		}
	}

	return node->position;
}

void readData() {
	ifstream fin("filtered_data.csv");

	string line;
	getline(fin, line);

	while (getline(fin, line)) {
		vector<string> row = splitCSV(line);

		if (row.size() < 13) continue;

		try {
			Player player;

			player.name = row[0];
			player.position = getPosition(row[1]);

			double minutes = stod(row[2]);

			if (minutes <= 0) continue;

			for (int i = 0; i < FEATURES; i++) {
				player.stat[i] =
					stod(row[i + 3]) * 90.0 / minutes;
			}

			players[playerCount++] = player;
		} catch (...) {
		}
	}
}

int main() {
	readData();

	shuffle(players, players + playerCount, rng);

	trainCount = playerCount * 6 / 7;
	testCount = playerCount - trainCount;

	for (int i = 0; i < trainCount; i++) trainData[i] = players[i];

	for (int i = 0; i < testCount; i++) testData[i] = players[trainCount + i];

	for (int tree = 0; tree < TREES; tree++) {
		int ids[MAXN];

		for (int i = 0; i < trainCount; i++) {
			ids[i] = rng() % trainCount;
		}

		forest[tree] = buildTree(ids, trainCount, 0);
	}

	int correct = 0;
	int confusion[3][3] = {};

	for (int i = 0; i < testCount; i++) {
		int votes[3] = {};

		for (int tree = 0; tree < TREES; tree++) {
			int prediction = predict(forest[tree], testData[i]);
			votes[prediction]++;
		}

		int answer = 0;

		for (int position = 1; position < 3; position++) {
			if (votes[position] > votes[answer]) {
				answer = position;
			}
		}

		if (answer == testData[i].position) {
			correct++;
		}

		confusion[testData[i].position][answer]++;

		cout << testData[i].name << ": "
		     << getPositionName(testData[i].position) << " -> "
		     << getPositionName(answer) << '\n';
	}

	cout << fixed << setprecision(2);

	cout << "\nAccuracy: "
	     << 100.0 * correct / testCount
	     << "%\n";

	cout << "\n    DEF MID FWD\n";

	for (int i = 0; i < 3; i++) {
		cout << getPositionName(i) << " ";

		for (int j = 0; j < 3; j++) {
			cout << setw(3) << confusion[i][j] << " ";
		}

		cout << '\n';
	}
}