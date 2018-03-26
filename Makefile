nlab:
	g++ --std=c++14 neuron.cpp tweann.cpp g_lab.cpp remote_env.cpp main.cpp -DNDEBUG -lpthread -O -o nlab

benchmark:
	g++ --std=c++14 benchmark/benchmark.cpp neuron.cpp tweann.cpp -I. -DNDEBUG -lpthread -lbenchmark -o benchmark/benchmark -O

clean:
	rm -f benchmark/benchmark nlab