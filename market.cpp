#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <iostream>
#include <fstream>


class limitOrder 
{
public:
	size_t agentID; // ID of agent who placed order
	size_t orderID; // Unique ID of order
	size_t size;
	float price;

	limitOrder(float price, size_t agentID, size_t size)
	{
		this->agentID = agentID;
		this->orderID = rand() % RAND_MAX;
		this->size = size;
		this->price = price;
	}
};

class orderPriceComparator
{
public:
	inline bool operator()(const limitOrder* a, const limitOrder* b)
	{
		return a->price < b->price;
	}
};

class limitBuy : public limitOrder
{
	using limitOrder::limitOrder;
};

class limitSell : public limitOrder
{
	using limitOrder::limitOrder;
};

void agent_callback_noddy(size_t agentID, size_t orderID);

// This class will store buy orders and sell orders.
class ledger
{
public:
	std::vector<limitBuy*> openBuy;
	std::vector<limitSell*> openSell;

	size_t previousBestAsk, previousBestBid;

	float getBestAsk()
	{
		if (openSell.empty())
			return previousBestAsk;

		return openSell[0]->price; // TODO: check which way we've sorted
	}

	float getBestBid()
	{
		if (openBuy.empty())
			return previousBestBid;

		return openBuy.back()->price;
	}

	void buy_at_market(size_t units)
	{
		size_t remaining_units = units;

		while (true)
		{
			if (remaining_units == 0 || openSell.empty())
				break;

			if (openSell.front()->size <= remaining_units)
			{
				// Decrease remaining_units
				// Notify the agent that it's order is complete
				// Remove the sell order from the books

				remaining_units -= openSell.front()->size;
				agent_callback_noddy(openSell.front()->agentID, openSell.front()->orderID);
				previousBestAsk = openSell.front()->price;
				delete openSell.front();
				openSell.erase(openSell.begin());
			}
			else
			{
				// Decrease size of open order
				// Decrease remaining_units to 0
				openSell.front()->size -= remaining_units;
				remaining_units = 0;
			}
		}
	}

	void sell_at_market(size_t units)
	{
		size_t remaining_units = units;

		while (true)
		{
			if (remaining_units == 0 || openBuy.empty())
				break;

			if (openBuy.back()->size <= remaining_units)
			{
				// Decrease remaining_units
				// Notify the agent that it's order is complete
				// Remove the sell order from the books

				remaining_units -= openBuy.back()->size;
				agent_callback_noddy(openBuy.back()->agentID, openBuy.back()->orderID);
				previousBestBid = openBuy.back()->price;
				delete openBuy.back();
				openBuy.erase(openBuy.end() - 1);
			}
			else
			{
				// Decrease size of open order
				// Decrease remaining_units to 0
				openBuy.back()->size -= remaining_units;
				remaining_units = 0;
			}
		}
	}

	// Returns order ID if successful, returns 0 if order was immediately filled.
	size_t add_limit_buy(float price, size_t agentID, size_t size)
	{
		auto order = new limitBuy(price, agentID, size);
		openBuy.push_back(order);
		std::sort(openBuy.begin(), openBuy.end(), orderPriceComparator());

		return order->orderID;
	}

	bool cancel_limit_buy(size_t orderID)
	{
		for (int i = 0; i < openBuy.size(); i++)
		{
			if (openBuy[i]->orderID == orderID)
			{
				delete openBuy[i];
				openBuy.erase(openBuy.begin() + i);
				std::sort(openBuy.begin(), openBuy.end(), orderPriceComparator());
				return false;
			}
		}

		return true;
	}

	// returns order ID if successful
	size_t add_limit_sell(float price, size_t agentID, size_t size)
	{
		auto order = new limitSell(price, agentID, size);
		openSell.push_back(order);
		std::sort(openSell.begin(), openSell.end(), orderPriceComparator());

		return order->orderID;
	}

	bool cancel_limit_sell(size_t orderID)
	{
		for (int i = 0; i < openSell.size(); i++)
		{
			if (openSell[i]->orderID == orderID)
			{
				delete openSell[i];
				openSell.erase(openSell.begin() + i);
				std::sort(openSell.begin(), openSell.end(), orderPriceComparator());
				return false;
			}
		}

		return true;
	}
};

class agent 
{
public:
	size_t agentID;
	bool buyer = true;
	virtual void act(ledger& l) = 0;
	std::vector<size_t> myOrders; // list of orderID's

	void set_buyer(bool buyer)
	{
		this->buyer = buyer;
	}
};

class impatientAgent : public agent
{
public:
	float ORDER_RATE = 1; // Mean orders per unit time
	float ORDER_SIZE_RATE = 40; // Mean shares per unit time

	std::random_device rd;
	std::mt19937 gen;
	std::poisson_distribution<> orderRateDistribution;
	std::poisson_distribution<> orderSizeRateDistribution;

	impatientAgent()
	{
		agentID = rand() % RAND_MAX;
		orderRateDistribution = std::poisson_distribution<>(ORDER_RATE);
		orderSizeRateDistribution = std::poisson_distribution<>(ORDER_SIZE_RATE);
		gen = std::mt19937(rd());
	}

	void act(ledger& l) override
	{
		// The act method for the impatient agent must:
		//	- Buy/sell a random number of shares at market

		size_t num_orders = orderRateDistribution(gen);
		for (int i = 0; i < num_orders; i++)
		{
			float units = orderSizeRateDistribution(gen);

			std::cout << "Impatient agent " << agentID << " buying " << units << " shares.\n";

			if (buyer)
				l.buy_at_market(units);
			else
				l.sell_at_market(units);
		}

		return;
	}
};

#include <chrono>

class patientAgent : public agent
{
public:
	float ORDER_RATE = 5; // Mean orders per unit time
	float ORDER_SIZE_RATE = 40; // Mean shares per order
	float CANCEL_RATE = 4; // Mean orders cancelled per unit time.

	std::random_device rd;
	std::mt19937 gen;

	// Order rate 
	std::poisson_distribution<> orderRateDistribution;

	// Order size rate 
	std::poisson_distribution<> orderSizeRateDistribution;

	// Order price
	std::uniform_int_distribution<> orderPriceDistribution;

	// Cancel rate
	std::poisson_distribution<> cancelRateDistribution;

	patientAgent()
	{
		agentID = rand() % RAND_MAX;

		gen = std::mt19937(rd());
		orderRateDistribution = std::poisson_distribution<>(ORDER_RATE);
		orderSizeRateDistribution = std::poisson_distribution<>(ORDER_SIZE_RATE);
		cancelRateDistribution = std::poisson_distribution<>(CANCEL_RATE);
	}

	// Called back when the ledged resolves our order to let us clean up our order book
	void resolve_order_callback(size_t orderID)
	{
		for (int i = 0; i < myOrders.size(); i++)
		{
			if (myOrders[i] == orderID)
				myOrders.erase(myOrders.begin() + i);
		}
	}

	void place_orders(ledger& l)
	{
		float units = orderSizeRateDistribution(gen);

		// ln(price) must be worse than best ask/best bid.

		int price;

		if (buyer)
		{
			float uniformUpperLimit = l.getBestAsk();
			orderPriceDistribution = std::uniform_int_distribution<>(0, uniformUpperLimit);
			price = orderPriceDistribution(gen);
			
			size_t orderID = l.add_limit_buy(price, agentID, units);
			
			if (orderID != 0)
				myOrders.push_back(orderID);


			std::cout << "Patient agent buying " << units << " shares at " << price << ".\n";
		}
		else // seller
		{
			float uniformLowerLimit = l.getBestBid();
			orderPriceDistribution = std::uniform_int_distribution<>(uniformLowerLimit, l.getBestBid() + 1000); // TODO: what should this be?
			price = orderPriceDistribution(gen);
			
			size_t orderID = l.add_limit_sell(price, agentID, units);

			if (orderID != 0)
				myOrders.push_back(orderID);

			std::cout << "Patient agent selling " << units << " shares at " << price << ".\n";
		}
	}

	void cancel_orders(ledger& l)
	{
		if (!myOrders.size())
			return;

		size_t orders = cancelRateDistribution(gen);

		orders = std::min(orders, myOrders.size() - 1);

		std::cout << "Cancelling " << orders << " orders.\n";

		// Cancel index
		std::uniform_int_distribution<> cancelIndexDistribution(0, myOrders.size() - 1);

		for (int i = 0; i < orders; i++)
		{
			// cancel a random order out of my_orders

			size_t index;
			
			while ((index = cancelIndexDistribution(gen)) >= myOrders.size())
				;

			if (buyer)
				l.cancel_limit_buy(myOrders[index]);
			else
				l.cancel_limit_sell(myOrders[index]);
			
			std::cout << "Patient agent cancelling order " << myOrders[index] << ".\n";

			myOrders.erase(myOrders.begin() + index);
		}
	}

	void act(ledger& l) override
	{
		// The act method for the patient agent must:
		//	- Buy/sell a random number of shares at a random price lower than best ask/best bid
		//	- Cancel open orders randomly

		size_t num_orders = orderRateDistribution(gen);
		for (int i = 0; i < num_orders; i++)
			place_orders(l);
		
		cancel_orders(l);
	}
};

static std::vector<agent*> agents;

void agent_callback_noddy(size_t agentID, size_t orderID)
{
	for (auto agent : agents)
	{
		if (agent->agentID == agentID)
		{
			for (auto order = agent->myOrders.begin(); order != agent->myOrders.end(); order++)
			{
				if (*order == orderID)
				{
					agent->myOrders.erase(order);
					return;
				}
			}
		}
	}
}

#define OBSERVER_ID 1100110011

int main()
{
	std::cout << "Hello market\n";

	ledger stockA;

	stockA.add_limit_buy(50, OBSERVER_ID, 40);
	stockA.add_limit_sell(100, OBSERVER_ID, 50);

	auto patient_buyer0 = new patientAgent();
	agents.push_back(patient_buyer0);

	auto patient_seller0 = new patientAgent();
	patient_seller0->set_buyer(false);
	agents.push_back(patient_seller0);

	auto patient_buyer1 = new patientAgent();
	agents.push_back(patient_buyer1);

	auto patient_seller1 = new patientAgent();
	patient_seller1->set_buyer(false);
	agents.push_back(patient_seller1);

	auto impatient_buyer = new impatientAgent();
	agents.push_back(impatient_buyer);

	auto impatient_seller = new impatientAgent();
	impatient_seller->set_buyer(false);
	agents.push_back(impatient_seller);

	std::ofstream myfile;
	myfile.open("example.csv");

	for (int i = 0; i < 1000; i++)
	{
		for (auto agent : agents)
		{
			agent->act(stockA);
		}

		auto ask = stockA.getBestAsk();
		auto bid = stockA.getBestBid();

		std::cout << "Best ask == " << ask
						<< ", best bid == " << bid << ".\n";

		myfile << ask << "," << bid << "\n";
	}

	myfile.close();

	return 0;
}