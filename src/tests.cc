// CSE 375/475 Assignment #1
// Fall 2020
//
// Description: This file implements a function 'run_custom_tests' that should be able to use
// the configuration information to drive tests that evaluate the correctness
// and performance of the map_t object.

#include <iostream>
#include <chrono>
#include "config_t.h"
#include "tests.h"
#include <future>
#include <thread>
#include <mutex>
#include <atomic>
#include <random>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "simplemap.h"

void printer(int k, float v) {
	std::cout<<"<"<<k<<","<<v<<">"<< std::endl;
}

// std::vector<std::pair<std::mutex, std::atomic<int>>> tex_list(256);
std::vector<std::pair<std::mutex, long>> tex_list(256);

void spinLock(int lock_id, int wait)
{
	while(!tex_list.at(lock_id).first.try_lock())
	{
		usleep(wait);
	}
}

void deposit(simplemap_t<int, double> *map, int from_account_key, int to_account_key, float amount)
{
	int from_lock_id = from_account_key % tex_list.size();
	int to_lock_id = to_account_key % tex_list.size();

	// if (from_lock_id < to_lock_id)
	// {
	// 	spinLock(from_lock_id, 7);
	// 	spinLock(to_lock_id, 11);
	// }
	// else if (from_lock_id > to_lock_id)
	// {
	// 	spinLock(to_lock_id, 13);
	// 	spinLock(from_lock_id, 17);
	// }
	// else
	// {
	// 	spinLock(from_lock_id, 19);
	// }

	if (from_lock_id < to_lock_id)
	{
		tex_list.at(from_lock_id).first.lock();
		tex_list.at(to_lock_id).first.lock();
	}
	else if (from_lock_id > to_lock_id)
	{
		tex_list.at(to_lock_id).first.lock();
		tex_list.at(from_lock_id).first.lock();
	}
	else
	{
		tex_list.at(from_lock_id).first.lock();
	}

	map->update(from_account_key, map->lookup(from_account_key).first - amount);
	map->update(to_account_key, map->lookup(to_account_key).first + amount);
	
	tex_list.at(from_lock_id).first.unlock();
	if (from_lock_id != to_lock_id)
	{
		tex_list.at(to_lock_id).first.unlock();
	}
}

 
double balance(simplemap_t<int, double> *map) {
	// for (int i = 0; i < tex_list.size(); i++)
	// {
	// 	while(!tex_list.at(i).first.try_lock() && tex_list.at(i).second == 0)
	// 	{
	// 		usleep(11);
	// 	}
	// 	tex_list.at(i).second++;
	// }

	double balance = map->sumAll();
	
	// for (int i = 0; i < tex_list.size(); i++)
	// {
	// 	tex_list.at(i).second--;

	// 	if (tex_list.at(i).second == 0)
	// 	{
	// 		tex_list.at(i).first.unlock();
	// 	}
	// }

	return balance;
}

unsigned do_work(simplemap_t<int, double> *map,
	std::vector<std::pair<bool, int>> random_list,
	std::vector<std::tuple<int,int,double>> deposit_list,
	int starting_index,
	int ending_index)
{
	std::chrono::time_point<std::chrono::high_resolution_clock> t1, t2;
	t1 = std::chrono::high_resolution_clock::now();
	for (int i = starting_index; i < ending_index; i++)
	{
		if (random_list.at(i).first)
		{
			double bal = balance(map);
			// printf("Total Balance: %.2f\n", bal);
		}
		else
		{
			std::tuple<int,int,double> deposit_schedule = deposit_list.at(random_list.at(i).second);
			deposit(map, std::get<0>(deposit_schedule), std::get<1>(deposit_schedule), std::get<2>(deposit_schedule));
		}
	}
	t2 = std::chrono::high_resolution_clock::now();
	return std::chrono::duration_cast<std::chrono::microseconds>( t2 - t1 ).count();
}

unsigned do_work_sequential(simplemap_t<int, double> *map,
	std::vector<std::pair<bool, int>> random_list,
	std::vector<std::tuple<int,int,double>> deposit_list,
	int starting_index,
	int ending_index)
{
	std::chrono::time_point<std::chrono::high_resolution_clock> t1, t2;
	t1 = std::chrono::high_resolution_clock::now();
	for (int i = starting_index; i < ending_index; i++)
	{
		if (random_list.at(i).first)
		{
			double bal = map->sumAll();
			// printf("Total Balance: %.2f\n", bal);
		}
		else
		{
			std::tuple<int,int,double> deposit_schedule = deposit_list.at(random_list.at(i).second);
			map->update(std::get<0>(deposit_schedule), map->lookup(std::get<0>(deposit_schedule)).first - std::get<2>(deposit_schedule));
			map->update(std::get<1>(deposit_schedule), map->lookup(std::get<1>(deposit_schedule)).first + std::get<2>(deposit_schedule));
		}
	}
	t2 = std::chrono::high_resolution_clock::now();
	return std::chrono::duration_cast<std::chrono::microseconds>( t2 - t1 ).count();
}

void run_custom_tests(config_t& cfg) {

	// Create and populate map
	simplemap_t<int, double> *map = new simplemap_t<int, double>();
	for (int i = 0; i < cfg.key_max - 1; i++)
	{
		map->insert(i, 100000/cfg.key_max);
	}
	map->insert(cfg.key_max - 1, 100000 - (cfg.key_max - 1) * (100000 / cfg.key_max));


	// Pre-generate randomness.
	std::mt19937 mtrand(time(NULL));
	std::vector<std::pair<bool, int>> random_list;
	std::vector<std::tuple<int,int,double>> deposit_list;

	for (int i = 0; i < cfg.iters; i++)
	{
		int random = std::uniform_int_distribution<int>(0, 19)(mtrand);
		random_list.push_back(std::make_pair(random == 0, deposit_list.size()));

		if (random != 0)
		{

			std::uniform_int_distribution<int> account_dist(0, cfg.key_max - 1);

			int from_account_key = account_dist(mtrand);
			int to_account_key = account_dist(mtrand);

			while (from_account_key == to_account_key)
			{
				to_account_key = account_dist(mtrand);
			}

			double amount = std::uniform_int_distribution<int>(0, 9999)(mtrand) / 100.00;

			deposit_list.push_back(std::make_tuple(from_account_key, to_account_key, amount));
		}
	}

	// Trigger parallel
	std::vector<std::future<unsigned>> futures = {};
	std::vector<std::thread> threads = {};

	int total = cfg.iters - (cfg.iters / cfg.threads) * cfg.threads;
	int start_index = 0;
	for (int i = 0; i < cfg.threads; i++)
	{
		// We use this to split up the job evenly
		int length = cfg.iters / cfg.threads + (total > 0);
		total -= (total > 0);

		std::packaged_task<unsigned()> task(std::bind(do_work, map, std::ref(random_list),std::ref(deposit_list), start_index, start_index + length)); // wrap the function
		std::future<unsigned> future = task.get_future();  // get a future
		std::thread new_thread(std::move(task)); // launch on a thread
		threads.push_back(std::move(new_thread));
		futures.push_back(std::move(future));
		start_index += length;
	}

	// Join threads
	unsigned max_thread_duration = 0;
	for (int i = 0; i < cfg.threads; i++)
	{
		if (threads.at(i).joinable())
		{
			threads.at(i).join();
		}
		max_thread_duration = std::max(max_thread_duration, futures.at(i).get());
	}

	std::cout << "Thread Duration Max: " << max_thread_duration << std::endl;
	std::cout << "Balance After Parallel: " << balance(map) << std::endl << std::endl;


	// Trigger sequential
	unsigned sequential_duration = 0;
	sequential_duration += do_work_sequential(map, random_list, deposit_list, 0, cfg.iters);

	std::cout << "Sequential Duration: " << sequential_duration << std::endl;
	std::cout << "Balance After Sequential: " << balance(map) << std::endl << std::endl;

	for (int i = 0; i < cfg.key_max; i++)
	{
		map->remove(i);
	}

	delete map;
}

void test_driver(config_t &cfg) {
	run_custom_tests(cfg);
}
