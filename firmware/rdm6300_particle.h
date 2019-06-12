/*
 * A simple library to interface with rdm6300 rfid reader.
 * This Particle port based on Arad Eizen's (https://github.com/arduino12/rdm6300)
 * library, which is licensed under GNU GPL V3.
*/

#define RDM6300_BAUDRATE		9600
#define RDM6300_PACKET_SIZE		14
#define RDM6300_PACKET_BEGIN	0x02
#define RDM6300_PACKET_END		0x03
#define RDM6300_NEXT_READ_MS	220
#define RDM6300_READ_TIMEOUT	20

class Rdm6300 {
	public:
		void init();
		bool update(void);
		uint32_t get_tag_id(void);
		bool is_tag_near(void);
	private:
		uint32_t _tag_id = 0;
		uint32_t _last_tag_id = 0;
		uint32_t _last_read_ms = 0;
};
