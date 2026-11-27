all: producer consumer

producer: producer.cpp
	g++ -o producer producer.cpp

consumer: consumer.cpp
	g++ -o consumer consumer.cpp

clean:
	rm -f producer consumer
	rm -f /dev/shm/shared_memory /dev/shm/sem.*