/*
 * A simple library to interface with rdm6300 rfid reader.
 * This Particle port based on Arad Eizen's (https://github.com/arduino12/rdm6300)
 * library, which is licensed under GNU GPL V3.
 */
#include <Particle.h>
#include "RDM6300_particle.h"

void Rdm6300::init() {
	Serial1.begin(RDM6300_BAUDRATE, SERIAL_8N1); // RDM6300 is picky about baudrate
	Serial1.setTimeout(RDM6300_READ_TIMEOUT);
}

bool Rdm6300::update(void) {

	char buffer[RDM6300_PACKET_SIZE];
	uint32_t tag_id;
	uint8_t checksum;

	if (!Serial1.available()) {
		return false;
	}

	// Drop any packet that doesn't begin with the right byte
	if (Serial1.peek() != RDM6300_PACKET_BEGIN && Serial1.read()) {
		return false;
	}

	// Drop any packet of the wrong size
	if (RDM6300_PACKET_SIZE != Serial1.readBytes(buffer, RDM6300_PACKET_SIZE)) {
		return false;
	}

	// Drop any packet that ends with the wrong byte
	if (buffer[13] != RDM6300_PACKET_END) {
		return false;
	}

	// Add null and parse checksum
	buffer[13] = 0;
	checksum = strtol(buffer + 11, NULL, 16);
	// Add null and parse current_tag_id
	buffer[11] = 0;
	tag_id = strtol(buffer + 3, NULL, 16);
	// Add null and parse version (needs to be xored with checksum)
	buffer[3] = 0;
	checksum ^= strtol(buffer + 1, NULL, 16);

	// xore the tag_id and validate checksum
	for (uint8_t i = 0; i < 32; i += 8)
		checksum ^= ((tag_id >> i) & 0xFF);
	if (checksum)
		return false;

	// If a new tag appears- return it
	if (_last_tag_id != tag_id) {
		_last_tag_id = tag_id;
		_last_read_ms = 0;
	}
	// If the old tag is still here set tag_id to zero
	if (is_tag_near())
		tag_id = 0;
	_last_read_ms = millis();

	_tag_id = tag_id;
	return tag_id;
}

bool Rdm6300::is_tag_near(void) {
	return millis() - _last_read_ms < RDM6300_NEXT_READ_MS;
}

uint32_t Rdm6300::get_tag_id(void) {
	uint32_t tag_id = _tag_id;
	_tag_id = 0;
	return tag_id;
}
