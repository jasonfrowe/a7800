// license:BSD-3-Clause
/**********************************************************************

    SaveKey: a 24LC256 I2C EEPROM (32K) on a controller port.

    The model: device select $A0/$A1 (A2-A0 tied low), two address bytes,
    64-byte page writes that wrap inside their page, sequential reads that
    run on through the whole array, ACK polling always ready (no 5 ms
    write cycle).

**********************************************************************/

#include "emu.h"
#include "savekey.h"

DEFINE_DEVICE_TYPE(A7800_SAVEKEY, a7800_savekey_device, "a7800_savekey", "SaveKey (24LC256 EEPROM)")

a7800_savekey_device::a7800_savekey_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock) :
	device_t(mconfig, A7800_SAVEKEY, tag, owner, clock),
	device_a7800_control_port_interface(mconfig, *this),
	device_nvram_interface(mconfig, *this),
	m_scl(1), m_sda(1), m_sdar(1), m_state(ST_IDLE), m_bit(0), m_acking(0), m_sending(0), m_shift(0), m_addr(0)
{
}

void a7800_savekey_device::device_start()
{
	m_mem = std::make_unique<uint8_t[]>(SIZE);
	save_pointer(NAME(m_mem.get()), SIZE);
	save_item(NAME(m_scl));
	save_item(NAME(m_sda));
	save_item(NAME(m_sdar));
	save_item(NAME(m_state));
	save_item(NAME(m_bit));
	save_item(NAME(m_acking));
	save_item(NAME(m_sending));
	save_item(NAME(m_shift));
	save_item(NAME(m_addr));
}

void a7800_savekey_device::device_reset()
{
	m_scl = m_sda = m_sdar = 1;
	m_state = ST_IDLE;
	m_bit = m_acking = m_sending = 0;
}

void a7800_savekey_device::nvram_default() { memset(m_mem.get(), 0xff, SIZE); }
void a7800_savekey_device::nvram_read(emu_file &file) { file.read(m_mem.get(), SIZE); }
void a7800_savekey_device::nvram_write(emu_file &file) { file.write(m_mem.get(), SIZE); }

// Only pin 3 (SDA) reads back; everything else idles high.
uint8_t a7800_savekey_device::a7800_joy_r()
{
	return (m_sdar && m_sda) ? 0xff : 0xfb;
}

void a7800_savekey_device::a7800_pins_w(uint8_t pins)
{
	int scl = BIT(pins, 3);
	int sda = BIT(pins, 2);

	if (sda != m_sda && m_scl && scl)
	{
		m_sda = sda;
		if (!sda)
		{   // start (or repeated start)
			m_state = ST_DEVSEL;
			m_bit = m_acking = m_sending = 0;
			m_sdar = 1;
		}
		else
		{   // stop
			m_state = ST_IDLE;
			m_acking = m_sending = 0;
			m_sdar = 1;
		}
	}
	m_sda = sda;

	if (scl != m_scl)
	{
		m_scl = scl;
		if (scl) scl_rise(); else scl_fall();
	}
}

void a7800_savekey_device::scl_rise()
{
	if (m_state == ST_IDLE)
		return;
	if (m_sending)
	{
		if (m_bit == 9)
		{   // the master's ACK (low) asks for another byte; NACK ends the read
			if (!m_sda)
			{
				m_addr = (m_addr + 1) & (SIZE - 1);
				m_shift = m_mem[m_addr];
				m_bit = 0;
			}
			else
			{   // (the address counter still moves on past the last byte)
				m_addr = (m_addr + 1) & (SIZE - 1);
				m_sending = 0;
				m_state = ST_IDLE;
			}
		}
		return;
	}
	if (!m_acking && m_bit < 8)
	{
		m_shift = (m_shift << 1) | m_sda;
		m_bit++;
	}
}

void a7800_savekey_device::scl_fall()
{
	if (m_state == ST_IDLE)
		return;
	if (m_sending)
	{
		if (m_bit < 8)
		{
			m_sdar = BIT(m_shift, 7 - m_bit);
			m_bit++;
		}
		else if (m_bit == 8)
		{
			m_sdar = 1;     // let go for the master's ACK
			m_bit = 9;
		}
		return;
	}
	if (m_acking)
	{   // the ACK clock is over
		m_acking = 0;
		m_sdar = 1;
		m_bit = 0;
		if (m_state == ST_READ)
		{
			m_sending = 1;
			m_shift = m_mem[m_addr];
			m_sdar = BIT(m_shift, 7);
			m_bit = 1;
		}
		return;
	}
	if (m_bit == 8)
		byte_in(m_shift);
}

void a7800_savekey_device::byte_in(uint8_t b)
{
	bool ack = true;
	switch (m_state)
	{
		case ST_DEVSEL:
			if ((b & 0xfe) == 0xa0)
				m_state = (b & 1) ? ST_READ : ST_ADDRH;
			else
				ack = false;
			break;
		case ST_ADDRH:
			m_addr = (b & 0x7f) << 8;
			m_state = ST_ADDRL;
			break;
		case ST_ADDRL:
			m_addr |= b;
			m_state = ST_WRITE;
			break;
		case ST_WRITE:
			m_mem[m_addr] = b;
			m_addr = (m_addr & ~(PAGE - 1)) | ((m_addr + 1) & (PAGE - 1));
			break;
	}
	if (ack)
	{
		m_sdar = 0;
		m_acking = 1;
	}
	else
	{
		m_state = ST_IDLE;
		m_sdar = 1;
		m_bit = 0;
	}
}
