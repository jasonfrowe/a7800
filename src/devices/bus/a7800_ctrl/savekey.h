// license:BSD-3-Clause
/**********************************************************************

    SaveKey: a 24LC256 I2C EEPROM (32K) on a controller port.

    SDA is pin 3 (bit 2) and SCL pin 4 (bit 3), driven open-drain by the
    console through the RIOT's data direction register, as AtariAge's
    SaveKey and the AtariVox (its EEPROM half) wire it. The contents
    persist in MAME's nvram directory.

**********************************************************************/

#ifndef MAME_BUS_A7800_CTRL_SAVEKEY_H
#define MAME_BUS_A7800_CTRL_SAVEKEY_H

#pragma once

#include "ctrl.h"


class a7800_savekey_device : public device_t,
							public device_a7800_control_port_interface,
							public device_nvram_interface
{
public:
	a7800_savekey_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

protected:
	virtual void device_start() override;
	virtual void device_reset() override;

	virtual void nvram_default() override;
	virtual void nvram_read(emu_file &file) override;
	virtual void nvram_write(emu_file &file) override;

	virtual uint8_t a7800_joy_r() override;
	virtual void a7800_pins_w(uint8_t pins) override;

private:
	enum { ST_IDLE, ST_DEVSEL, ST_ADDRH, ST_ADDRL, ST_WRITE, ST_READ };
	static constexpr int SIZE = 0x8000;
	static constexpr int PAGE = 64;

	void scl_rise();
	void scl_fall();
	void byte_in(uint8_t b);

	std::unique_ptr<uint8_t[]> m_mem;
	int m_scl, m_sda;       // the lines as the console drives them (1 = released)
	int m_sdar;             // the EEPROM's output on SDA (1 = released)
	int m_state;
	int m_bit;              // bits received / sent in this byte
	int m_acking;           // the EEPROM is holding its ACK for this clock
	int m_sending;          // transmitting a byte (reads)
	uint8_t m_shift;
	uint16_t m_addr;
};

DECLARE_DEVICE_TYPE(A7800_SAVEKEY, a7800_savekey_device)

#endif // MAME_BUS_A7800_CTRL_SAVEKEY_H
