#ifndef __DINETWORK_H__
#define __DINETWORK_H__

class netdev;

class device_network_interface : public device_interface
{
public:
	device_network_interface(const machine_config &mconfig, device_t &device, float bandwidth);
	virtual ~device_network_interface();

	void set_interface(const char *name);
	void set_promisc(bool promisc);
	void set_mac(const char *mac);

	const char *get_mac() { return m_mac; }
	bool get_promisc() { return m_promisc; }

	int send(UINT8 *buf, int len);
	virtual void recv_cb(UINT8 *buf, int len);
	virtual bool mcast_chk(const UINT8 *buf, int len);

protected:
	bool m_promisc;
	char m_mac[6];
	float m_bandwidth;
	class netdev *m_dev;
};
#endif
