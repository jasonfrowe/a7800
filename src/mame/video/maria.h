// license:BSD-3-Clause
// copyright-holders:Dan Boris
#ifndef MAME_VIDEO_MARIA_H
#define MAME_VIDEO_MARIA_H

#pragma once


// ======================> atari_maria_device

class atari_maria_device :  public device_t
{
public:
	// construction/destruction
	atari_maria_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	static void set_cpu_tag(device_t &device, const char *tag) { downcast<atari_maria_device &>(device).m_cpu_tag = tag; }

	void interrupt(int lines);
	void startdma(int lines);
	void display_visible(int pixelx);

	int m_devmode_flag;
	int m_devmode_dmacolor;
	int m_devmode_dmacolorwidth;

	// Development statistics, read by the driver (outputs, logs, profiler).
	// MARIA cycles are its own (4 to a CPU cycle); "stolen" cycles are CPU
	// cycles the CPU spent halted for DMA (not counting lines on which it was
	// already waiting in WSYNC).
	uint64_t m_stat_stolen;            // stolen CPU cycles since power-on
	int m_stat_frames;                 // frames completed
	// the last completed frame:
	int m_stat_cycles;                 // MARIA DMA cycles, all lines
	int m_stat_stolen_frame;           // stolen CPU cycles
	int m_stat_over;                   // lines that hit the DMA limit (cut short)
	int m_stat_peak;                   // the busiest line's MARIA cycles
	int m_stat_peak_line;              // ... and that line (0 = first visible)
	int m_stat_lines[243];             // each visible line's MARIA cycles
	static constexpr int STAT_LIMIT = 430;

	DECLARE_READ8_MEMBER(read);
	DECLARE_WRITE8_MEMBER(write);

	uint32_t screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);

protected:
	// device-level overrides
	virtual void device_start() override;
	virtual void device_reset() override;

private:

	int m_maria_palette[32];
	int m_line_ram[2][160];
	int m_active_buffer;
	int m_write_mode;
	unsigned int m_dll;
	unsigned int m_dl;
	int m_holey;
	int m_offset;
	int m_vblank;
	int m_dmaon;
	int m_dpp;
	int m_wsync;
	int m_color_kill;
	int m_cwidth;
	int m_bcntl;
	int m_kangaroo;
	int m_rm;
	int m_nmi;
	unsigned int m_charbase;
	bitmap_ind16 m_bitmap;

	int m_acc_cycles, m_acc_stolen, m_acc_over, m_acc_peak, m_acc_peak_line;
	int m_acc_lines[243];
	int m_stat_vline;

	void draw_scanline();
	void stat_frame_done();
	int is_holey(unsigned int addr);
	int write_line_ram(int addr, uint8_t offset, int pal);

	const char *m_cpu_tag;
	cpu_device *m_cpu;  // CPU whose space(AS_PROGRAM) serves as DMA source
	screen_device *m_screen;
};


// device type definition
DECLARE_DEVICE_TYPE(ATARI_MARIA, atari_maria_device)


#define MCFG_MARIA_DMACPU(_tag) \
	atari_maria_device::set_cpu_tag(*device, _tag);


#endif // MAME_VIDEO_MARIA_H
