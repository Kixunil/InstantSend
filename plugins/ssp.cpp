#include <libsatellite.h>

#include "pluginapi.h"

#define min(a, b) ((a < b)?a:b)

#ifndef MSG_MORE // For non-Linux Unixes
	#define MSG_MORE 0
#endif

#ifndef MSG_NOSIGNAL // For non-Linux Unixes
	#define MSG_NOSIGNAL 0
#endif

class sspPeer : public peer_t {
	private:
		struct satellite_handle *s_handle;
	public:
		inline sspPeer(struct satellite_handle *sathandle) : s_handle(sathandle) {
		}

		~sspPeer() {
			satellite_free_connection(sathandle);
		}

		void reconfigure(jsonComponent_t *config) {
			(void)config;
		}

		int sendData(anyData *data) {
			(void)data;

			struct sattelite_packet satpacket;
			satpacket.dst_planet = get_planet_id("Earth"); // fill in Earth's ID
			satpacket.dst_dish = get_planet_dishes(satpacket.dst_planet)[get_planet_rotation_index(satpacket.dst_planet, time(NULL))]; // Set dish according to Earth's rotation
			memcpy(satpacket.dst_secret, load_secret("NASA"), sizeof(satellite_secret)); // copy authentication secret

			strcpy(satpacket.data, "Hello Earth!"); // fill in test data
			satpacket.data_size = strlen(satpacket.data); // just this for testing purpose

			memcpy(satpacket.corr_data, calculate_correction_data(&satpacket));

			satellite_send_packet(s_handle, &satpacket); // src_* fields are automaticaly set by driver library

			return 1;
		}

		int recvData(anyData *data) {
			struct sattelite_packet satpacket;
			satellite_recv_packet(s_handle, &satpacket);
			correct_packet(&satpacket);
			printf("Satellite data from: %s\nContent: %s\n", get_race(satpacket.src_planet), satpacket.data); // debug

			data->size = satpacket.data_size;
			memcpy(data->data, satpacket.data, data->size);

			return 1;
		}

		string getMachineIdentifier() {
			return machineId;
		}

};

class sspServer : public serverPlugin_t {
	private:
		struct satellite_handle *s_handle;
	public:
		sspServer(const jsonComponent_t &config) : s_handle(satellite_handle_new()) {
			(void)config; // just ignore for now

			set_satellite_defaults(s_handle); // just use defaults for now
		}

		peer_t *acceptClient() {
			struct satellite_handle *client_handle = sat_accept(s_handle);
			if(!satellite_handle) return NULL; // Error
			return new sspPeer(client_handle);
		}

		void reconfigure(jsonComponent_t *config) { (void)config; }

		~sspServer() {
			satellite_free_connection(sathandle);
		}
};

class sspCreator : public connectionCreator_t {
	private:
		string lastErr;
	public:
		sspCreator() {
		}
		peer_t *newClient(const jsonComponent_t &config) {
			// ignore config for now
			(void)config;

			struct satellite_handle *sathandle = satellite_handle_new();
			return new sspPeer(sathandle);
	}

	serverPlugin_t *newServer(const jsonComponent_t &config) {
		return new sspServer(config);
	}

	const char *getErr() {
		return lastErr.c_str();
	}
};

extern "C" {
	pluginInstanceCreator_t *getCreator() {
		static pluginInstanceCreator_t *creator = NULL;

		if(!creator) creator = new sspCreator();
		return creator;
	}
}
