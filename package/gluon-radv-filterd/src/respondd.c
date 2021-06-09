#include <respondd.h>

#include <json-c/json.h>
#include <libgluonutil.h>
#include <net/ethernet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "mac.h"

static struct json_object * get_radv_filter() {
	FILE *f = popen("exec ebtables-tiny -L RADV_FILTER", "r");
	char *line = NULL;
	size_t len = 0;
	struct ether_addr mac = {};
	struct ether_addr unspec = {};
	char macstr[F_MAC_LEN + 1] = "";

	if (!f)
		return NULL;

	while (getline(&line, &len, f) > 0) {
		if (sscanf(line, "-s " F_MAC " -j ACCEPT\n", F_MAC_VAR_REF(mac)) == ETH_ALEN)
			break;
	}
	free(line);

	pclose(f);

	memset(&unspec, 0, sizeof(unspec));
	if (ether_addr_equal(mac, unspec)) {
		return NULL;
	} else {
		snprintf(macstr, sizeof(macstr), F_MAC, F_MAC_VAR(mac));
		return gluonutil_wrap_string(macstr);
	}
}

static struct json_object * respondd_provider_statistics() {
	struct json_object *ret = json_object_new_object();
	char buf[6];
	int fd = open("/sys/class/net/bat0/mesh/gw_mode", O_RDONLY);
	if (fd == -1) {
		perror("error opening gateway info file");
		return ret;
	}
	memset(buf, 0, sizeof(buf));
	read(fd, buf, sizeof(buf));
	close(fd);
	if (strncmp(buf, "server", 6) == 0) {
		// We are a batman gateway, do not write gateway6
		return ret;
	}

	json_object_object_add(ret, "gateway6", get_radv_filter());

	return ret;
}

const struct respondd_provider_info respondd_providers[] = {
	{"statistics", respondd_provider_statistics},
	{}
};
