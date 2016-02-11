#include <stdio.h>
#include <poll.h>
#include <net/netmap_user.h>

#define NETMAP_WITH_LIBS

int number_of_packets = 0;

void usage
{
	printf("usage : a./out -i netmap:<network interface>/<flag>\n");
	printf("\t\tex) $ ./a.out -i netmap:ix0/rt");
}


int main(int argc, char *argv[]) 
{
	int c;
    char interface[] = "";

	while ((c = getopt(argc, argv, "i:")) != -1) {
		switch (c) {
			case 'i' :	interface = optarg;
						break;
		}
	}

	if (interface == NULL) {
		printf("Error: interface is not specified!");
		exit(1);
	}

    struct nm_desc* netmap_descriptor;

    u_int num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
    printf("We have %d cpus\n", num_cpus);

    struct nmreq base_nmd;
    bzero(&base_nmd, sizeof(base_nmd));

    base_nmd.nr_tx_rings = base_nmd.nr_rx_rings = 0;
    base_nmd.nr_tx_slots = base_nmd.nr_rx_slots = 0;

    netmap_descriptor = nm_open(interface, &base_nmd, 0, NULL);

    if (netmap_descriptor == NULL) {
        printf("Can't open netmap device %s\n", interface);
        exit(1);
        return 0;
    }

    printf("Mapped %dKB memory at %p\n", netmap_descriptor->req.nr_memsize >> 10, netmap_descriptor->mem);
    printf("We have %d tx and %d rx rings\n", netmap_descriptor->req.nr_tx_rings,
           netmap_descriptor->req.nr_rx_rings);

    /*
        protocol stack and may cause a reset of the card,
        which in turn may take some time for the PHY to
        reconfigure. We do the open here to have time to reset.
    */

    int wait_link = 2;
    printf("Wait %d seconds for NIC reset\n", wait_link);
    sleep(wait_link);

    for (int i = 0; i < num_cpus; i++) {
        struct nm_desc nmd = *netmap_descriptor;
        // This operation is VERY important!
        nmd.self = &nmd;

        uint64_t nmd_flags = 0;

        nmd.req.nr_flags = NR_REG_ONE_NIC;
        nmd.req.nr_ringid = i;

        /* Only touch one of the rings (rx is already ok) */
        nmd_flags |= NETMAP_NO_TX_POLL;

        struct nm_desc* new_nmd =
        nm_open(interface, NULL, nmd_flags | NM_OPEN_IFNAME | NM_OPEN_NO_MMAP, &nmd);

        if (new_nmd == NULL) {
            printf("Can't open netmap descripto for netmap\n");
            exit(1);
        }

        printf("My first ring is %d and last ring id is %d I'm thread %d\n", new_nmd->first_rx_ring,
               new_nmd->last_rx_ring, i);

        printf("Start new thread %d\n", i);
        // Start thread and pass netmap descriptor to it
    }

    printf("Wait for thread finish\n");
    // Wait all threads for completion
    //for (int i = 0; i < num_cpus; i++) {
    //    boost_threads_array[i]->join();
    //}
    // receiver();


}
