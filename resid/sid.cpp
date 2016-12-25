//  ---------------------------------------------------------------------------
//  This file is part of reSID, a MOS6581 SID emulator engine.
//  Copyright (C) 2010  Dag Lem <resid@nimrod.no>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//  ---------------------------------------------------------------------------

#include "plus4emu.hpp"
#include "sid.hpp"
#include <cmath>

namespace Plus4 {

  // --------------------------------------------------------------------------
  // Constructor.
  // --------------------------------------------------------------------------
  SID::SID(int32_t& soundOutputAccumulator_)
    : soundOutputAccumulator(soundOutputAccumulator_)
  {
    sid_model = MOS6581;
    voice[0].set_sync_source(&voice[2]);
    voice[1].set_sync_source(&voice[0]);
    voice[2].set_sync_source(&voice[1]);

    bus_value = 0;
    bus_value_ttl = 0;
    write_pipeline = 0;

    databus_ttl = 0;
  }

  // --------------------------------------------------------------------------
  // Destructor.
  // --------------------------------------------------------------------------
  SID::~SID()
  {
  }

  // --------------------------------------------------------------------------
  // Set chip model.
  // --------------------------------------------------------------------------
  void SID::set_chip_model(chip_model model)
  {
    sid_model = model;

    /*
      results from real C64 (testprogs/SID/bitfade/delayfrq0.prg):

      (new SID) (250469/8580R5) (250469/8580R5)
      delayfrq0    ~7a000        ~108000

      (old SID) (250407/6581)
      delayfrq0    ~01d00

     */
#if 0
    databus_ttl = sid_model == MOS8580 ? 0xa2000 : 0x1d00;
#else
    // corrected values for TED clock frequency
    databus_ttl = sid_model == MOS8580 ?
                  int(0.663552 * RESID_CLOCK_FREQUENCY + 0.5)
                  : int(0.007424 * RESID_CLOCK_FREQUENCY + 0.5);
#endif

    for (int i = 0; i < 3; i++) {
      voice[i].set_chip_model(model);
    }

    filter.set_chip_model(model);
  }

  // --------------------------------------------------------------------------
  // SID reset.
  // --------------------------------------------------------------------------
  void SID::reset()
  {
    for (int i = 0; i < 3; i++) {
      voice[i].reset();
    }
    filter.reset();
    extfilt.reset();

    bus_value = 0;
    bus_value_ttl = 0;
  }

  // --------------------------------------------------------------------------
  // Write 16-bit sample to audio input.
  // Note that to mix in an external audio signal, the signal should be
  // resampled to 1MHz first to avoid sampling noise.
  // --------------------------------------------------------------------------
  void SID::input(short sample)
  {
    // The input can be used to simulate the MOS8580 "digi boost" hardware hack.
    filter.input(sample);
  }

  // --------------------------------------------------------------------------
  // Read registers.
  //
  // Reading a write only register returns the last byte written to any SID
  // register. The individual bits in this value start to fade down towards
  // zero after a few cycles. All bits reach zero within approximately
  // $2000 - $4000 cycles.
  // It has been claimed that this fading happens in an orderly fashion,
  // however sampling of write only registers reveals that this is not the
  // case.
  // NB! This is not correctly modeled.
  // The actual use of write only registers has largely been made in the belief
  // that all SID registers are readable. To support this belief the read
  // would have to be done immediately after a write to the same register
  // (remember that an intermediate write to another register would yield that
  // value instead). With this in mind we return the last value written to
  // any SID register for $4000 cycles without modeling the bit fading.
  // --------------------------------------------------------------------------
  reg8 SID::read(reg8 offset)
  {
    switch (offset) {
    case 0x19:
      bus_value = potx.readPOT();
      bus_value_ttl = databus_ttl;
      break;
    case 0x1a:
      bus_value = poty.readPOT();
      bus_value_ttl = databus_ttl;
      break;
    case 0x1b:
      bus_value = voice[2].wave.readOSC();
      bus_value_ttl = databus_ttl;
      break;
    case 0x1c:
      bus_value = voice[2].envelope.readENV();
      bus_value_ttl = databus_ttl;
      break;
    }
    return bus_value;
  }

  // --------------------------------------------------------------------------
  // Write registers.
  // Writes are one cycle delayed on the MOS8580. This is only modeled for
  // single cycle clocking.
  // --------------------------------------------------------------------------
  void SID::write(reg8 offset, reg8 value)
  {
    write_address = offset;
    bus_value = value;
    bus_value_ttl = databus_ttl;

    if (sid_model == MOS8580) {
      write_pipeline = 1;
    }
    else {
      write();
    }
  }

  // --------------------------------------------------------------------------
  // Write registers.
  // --------------------------------------------------------------------------
  void SID::write()
  {
    switch (write_address) {
    case 0x00:
      voice[0].wave.writeFREQ_LO(bus_value);
      break;
    case 0x01:
      voice[0].wave.writeFREQ_HI(bus_value);
      break;
    case 0x02:
      voice[0].wave.writePW_LO(bus_value);
      break;
    case 0x03:
      voice[0].wave.writePW_HI(bus_value);
      break;
    case 0x04:
      voice[0].writeCONTROL_REG(bus_value);
      break;
    case 0x05:
      voice[0].envelope.writeATTACK_DECAY(bus_value);
      break;
    case 0x06:
      voice[0].envelope.writeSUSTAIN_RELEASE(bus_value);
      break;
    case 0x07:
      voice[1].wave.writeFREQ_LO(bus_value);
      break;
    case 0x08:
      voice[1].wave.writeFREQ_HI(bus_value);
      break;
    case 0x09:
      voice[1].wave.writePW_LO(bus_value);
      break;
    case 0x0a:
      voice[1].wave.writePW_HI(bus_value);
      break;
    case 0x0b:
      voice[1].writeCONTROL_REG(bus_value);
      break;
    case 0x0c:
      voice[1].envelope.writeATTACK_DECAY(bus_value);
      break;
    case 0x0d:
      voice[1].envelope.writeSUSTAIN_RELEASE(bus_value);
      break;
    case 0x0e:
      voice[2].wave.writeFREQ_LO(bus_value);
      break;
    case 0x0f:
      voice[2].wave.writeFREQ_HI(bus_value);
      break;
    case 0x10:
      voice[2].wave.writePW_LO(bus_value);
      break;
    case 0x11:
      voice[2].wave.writePW_HI(bus_value);
      break;
    case 0x12:
      voice[2].writeCONTROL_REG(bus_value);
      break;
    case 0x13:
      voice[2].envelope.writeATTACK_DECAY(bus_value);
      break;
    case 0x14:
      voice[2].envelope.writeSUSTAIN_RELEASE(bus_value);
      break;
    case 0x15:
      filter.writeFC_LO(bus_value);
      break;
    case 0x16:
      filter.writeFC_HI(bus_value);
      break;
    case 0x17:
      filter.writeRES_FILT(bus_value);
      break;
    case 0x18:
      filter.writeMODE_VOL(bus_value);
      break;
    default:
      break;
    }

    // Tell clock() that the pipeline is empty.
    write_pipeline = 0;
  }

  // --------------------------------------------------------------------------
  // Constructor.
  // --------------------------------------------------------------------------
  SID::State::State()
  {
    int i;

    for (i = 0; i < 0x20; i++) {
      sid_register[i] = 0;
    }

    bus_value = 0;
    bus_value_ttl = 0;
    write_pipeline = 0;
    write_address = 0;
    voice_mask = 0xff;

    for (i = 0; i < 3; i++) {
      accumulator[i] = 0;
      shift_register[i] = 0x7fffff;
      shift_register_reset[i] = 0;
      shift_pipeline[i] = 0;
      pulse_output[i] = 0;
      floating_output_ttl[i] = 0;

      rate_counter[i] = 0;
      rate_counter_period[i] = 9;
      exponential_counter[i] = 0;
      exponential_counter_period[i] = 1;
      envelope_counter[i] = 0;
      envelope_state[i] = EnvelopeGenerator::RELEASE;
      hold_zero[i] = true;
      envelope_pipeline[i] = 0;
    }
  }

  // --------------------------------------------------------------------------
  // Read state.
  // --------------------------------------------------------------------------
  SID::State SID::read_state()
  {
    State state;
    int i, j;

    for (i = 0, j = 0; i < 3; i++, j += 7) {
      WaveformGenerator& wave = voice[i].wave;
      EnvelopeGenerator& envelope = voice[i].envelope;
      state.sid_register[j + 0] = wave.freq & 0xff;
      state.sid_register[j + 1] = wave.freq >> 8;
      state.sid_register[j + 2] = wave.pw & 0xff;
      state.sid_register[j + 3] = wave.pw >> 8;
      state.sid_register[j + 4] =
        (wave.waveform << 4)
        | (wave.test ? 0x08 : 0)
        | (wave.ring_mod ? 0x04 : 0)
        | (wave.sync ? 0x02 : 0)
        | (envelope.gate ? 0x01 : 0);
      state.sid_register[j + 5] = (envelope.attack << 4) | envelope.decay;
      state.sid_register[j + 6] = (envelope.sustain << 4) | envelope.release;
    }

    state.sid_register[j++] = filter.fc & 0x007;
    state.sid_register[j++] = filter.fc >> 3;
    state.sid_register[j++] = (filter.res << 4) | filter.filt;
    state.sid_register[j++] = filter.mode | filter.vol;

    // These registers are superfluous, but are included for completeness.
    for (; j < 0x1d; j++) {
      state.sid_register[j] = read(j);
    }
    for (; j < 0x20; j++) {
      state.sid_register[j] = 0;
    }

    state.bus_value = bus_value;
    state.bus_value_ttl = bus_value_ttl;
    state.write_pipeline = write_pipeline;
    state.write_address = write_address;
    state.voice_mask = filter.voice_mask;

    for (i = 0; i < 3; i++) {
      state.accumulator[i] = voice[i].wave.accumulator;
      state.shift_register[i] = voice[i].wave.shift_register;
      state.shift_register_reset[i] = voice[i].wave.shift_register_reset;
      state.shift_pipeline[i] = voice[i].wave.shift_pipeline;
      state.pulse_output[i] = voice[i].wave.pulse_output;
      state.floating_output_ttl[i] = voice[i].wave.floating_output_ttl;

      state.rate_counter[i] = voice[i].envelope.rate_counter;
      state.rate_counter_period[i] = voice[i].envelope.rate_period;
      state.exponential_counter[i] = voice[i].envelope.exponential_counter;
      state.exponential_counter_period[i] =
          voice[i].envelope.exponential_counter_period;
      state.envelope_counter[i] = voice[i].envelope.envelope_counter;
      state.envelope_state[i] = voice[i].envelope.state;
      state.hold_zero[i] = voice[i].envelope.hold_zero;
      state.envelope_pipeline[i] = voice[i].envelope.envelope_pipeline;
    }

    return state;
  }

  // --------------------------------------------------------------------------
  // Write state.
  // --------------------------------------------------------------------------
  void SID::write_state(const State& state)
  {
    int i;

    for (i = 0; i <= 0x18; i++) {
      write(i, state.sid_register[i]);
    }

    bus_value = state.bus_value;
    bus_value_ttl = state.bus_value_ttl;
    write_pipeline = state.write_pipeline;
    write_address = state.write_address;
    filter.set_voice_mask(state.voice_mask);

    for (i = 0; i < 3; i++) {
      voice[i].wave.accumulator = state.accumulator[i];
      voice[i].wave.shift_register = state.shift_register[i];
      voice[i].wave.shift_register_reset = state.shift_register_reset[i];
      voice[i].wave.shift_pipeline = state.shift_pipeline[i];
      voice[i].wave.pulse_output = state.pulse_output[i];
      voice[i].wave.floating_output_ttl = state.floating_output_ttl[i];

      voice[i].envelope.rate_counter = state.rate_counter[i];
      voice[i].envelope.rate_period = state.rate_counter_period[i];
      voice[i].envelope.exponential_counter = state.exponential_counter[i];
      voice[i].envelope.exponential_counter_period =
          state.exponential_counter_period[i];
      voice[i].envelope.envelope_counter = state.envelope_counter[i];
      voice[i].envelope.state = state.envelope_state[i];
      voice[i].envelope.hold_zero = state.hold_zero[i];
      voice[i].envelope.envelope_pipeline = state.envelope_pipeline[i];
    }
  }

  // --------------------------------------------------------------------------
  // Mask for voices routed into the filter / audio output stage.
  // Used to physically connect/disconnect EXT IN, and for test purposed
  // (voice muting).
  // --------------------------------------------------------------------------
  void SID::set_voice_mask(reg4 mask)
  {
    filter.set_voice_mask(mask);
  }

  // --------------------------------------------------------------------------
  // Enable filter.
  // --------------------------------------------------------------------------
  void SID::enable_filter(bool enable)
  {
    filter.enable_filter(enable);
  }

  // --------------------------------------------------------------------------
  // Adjust the DAC bias parameter of the filter.
  // This gives user variable control of the exact CF -> center frequency
  // mapping used by the filter.
  // The setting is currently only effective for 6581.
  // --------------------------------------------------------------------------
  void SID::adjust_filter_bias(double dac_bias)
  {
    filter.adjust_filter_bias(dac_bias);
  }

  // --------------------------------------------------------------------------
  // Enable external filter.
  // --------------------------------------------------------------------------
  void SID::enable_external_filter(bool enable)
  {
    extfilt.enable_filter(enable);
  }

  // --------------------------------------------------------------------------
  // SID clocking - 1 cycle.
  // --------------------------------------------------------------------------
  PLUS4EMU_INLINE void SID::clock_fast()
  {
    int i;

    // Clock amplitude modulators.
    for (i = 0; i < 3; i++) {
      voice[i].envelope.clock();
    }

    // Clock oscillators.
    for (i = 0; i < 3; i++) {
      voice[i].wave.clock();
    }

    // Synchronize oscillators.
    for (i = 0; i < 3; i++) {
      voice[i].wave.synchronize();
    }

    // Calculate waveform output.
    for (i = 0; i < 3; i++) {
      voice[i].wave.set_waveform_output();
    }

    // Clock filter.
    filter.clock(voice[0].output(), voice[1].output(), voice[2].output());

    soundOutputAccumulator =
        soundOutputAccumulator + (int32_t(filter.output()) * 15 - 131072);

    // Pipelined writes on the MOS8580.
    if (PLUS4EMU_UNLIKELY(write_pipeline)) {
      write();
    }

    // Age bus value.
    if (PLUS4EMU_UNLIKELY(!--bus_value_ttl)) {
      bus_value = 0;
    }
  }

  // --------------------------------------------------------------------------
  // SID clocking - 1 cycle.
  // --------------------------------------------------------------------------
  PLUS4EMU_REGPARM1 void SID::clockCallback(void *userData)
  {
    reinterpret_cast< SID * >(userData)->clock_fast();
  }

  // --------------------------------------------------------------------------
  // SID clocking - delta_t cycles.
  // --------------------------------------------------------------------------
  void SID::clock(cycle_count delta_t)
  {
    int i;

    // Pipelined writes on the MOS8580.
    if (PLUS4EMU_UNLIKELY(write_pipeline) && PLUS4EMU_EXPECT(delta_t > 0)) {
      // Step one cycle by a recursive call to ourselves.
      write_pipeline = 0;
      clock(1);
      write();
      delta_t -= 1;
    }

    if (PLUS4EMU_UNLIKELY(delta_t <= 0)) {
      return;
    }

    // Age bus value.
    bus_value_ttl -= delta_t;
    if (PLUS4EMU_UNLIKELY(bus_value_ttl <= 0)) {
      bus_value = 0;
      bus_value_ttl = 0;
    }

    // Clock amplitude modulators.
    for (i = 0; i < 3; i++) {
      voice[i].envelope.clock(delta_t);
    }

    // Clock and synchronize oscillators.
    // Loop until we reach the current cycle.
    cycle_count delta_t_osc = delta_t;
    while (delta_t_osc) {
      cycle_count delta_t_min = delta_t_osc;

      // Find minimum number of cycles to an oscillator accumulator MSB toggle.
      // We have to clock on each MSB on / MSB off for hard sync to operate
      // correctly.
      for (i = 0; i < 3; i++) {
        WaveformGenerator& wave = voice[i].wave;

        // It is only necessary to clock on the MSB of an oscillator that is
        // a sync source and has freq != 0.
        if (PLUS4EMU_EXPECT(!(wave.sync_dest->sync && wave.freq))) {
          continue;
        }

        reg16 freq = wave.freq;
        reg24 accumulator = wave.accumulator;

        // Clock on MSB off if MSB is on, clock on MSB on if MSB is off.
        reg24 delta_accumulator =
          (accumulator & 0x800000 ? 0x1000000 : 0x800000) - accumulator;

        cycle_count delta_t_next = delta_accumulator/freq;
        if (PLUS4EMU_EXPECT(bool(delta_accumulator%freq))) {
          ++delta_t_next;
        }

        if (PLUS4EMU_UNLIKELY(delta_t_next < delta_t_min)) {
          delta_t_min = delta_t_next;
        }
      }

      // Clock oscillators.
      for (i = 0; i < 3; i++) {
        voice[i].wave.clock(delta_t_min);
      }

      // Synchronize oscillators.
      for (i = 0; i < 3; i++) {
        voice[i].wave.synchronize();
      }

      delta_t_osc -= delta_t_min;
    }

    // Calculate waveform output.
    for (i = 0; i < 3; i++) {
      voice[i].wave.set_waveform_output(delta_t);
    }

    // Clock filter.
    filter.clock(delta_t,
                 voice[0].output(), voice[1].output(), voice[2].output());

    // Clock external filter.
    extfilt.clock(delta_t, filter.output());
  }

  // --------------------------------------------------------------------------

  class ChunkType_SIDSnapshot : public Plus4Emu::File::ChunkTypeHandler {
   private:
    SID&    ref;
   public:
    ChunkType_SIDSnapshot(SID& ref_)
      : Plus4Emu::File::ChunkTypeHandler(),
        ref(ref_)
    {
    }
    virtual ~ChunkType_SIDSnapshot()
    {
    }
    virtual Plus4Emu::File::ChunkType getChunkType() const
    {
      return Plus4Emu::File::PLUS4EMU_CHUNKTYPE_SID_STATE;
    }
    virtual void processChunk(Plus4Emu::File::Buffer& buf)
    {
      ref.loadState(buf);
    }
  };

  void SID::saveState(Plus4Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    buf.writeUInt32(0x01000000);        // version number
    // TODO: update snapshot format for new reSID version
    State   state_ = read_state();
    for (uint8_t i = 0x00; i <= 0x1F; i++)
      buf.writeByte(uint8_t(state_.sid_register[i]));
    buf.writeByte(uint8_t(state_.bus_value));
    buf.writeInt32(int32_t(state_.bus_value_ttl));
    for (uint8_t i = 0; i < 3; i++) {
      buf.writeUInt32(state_.accumulator[i]);
      buf.writeUInt32(state_.shift_register[i]);
      buf.writeUInt32(state_.rate_counter[i]);
      buf.writeUInt32(state_.rate_counter_period[i]);
      buf.writeUInt32(state_.exponential_counter[i]);
      buf.writeUInt32(state_.exponential_counter_period[i]);
      buf.writeByte(uint8_t(state_.envelope_counter[i]));
      if (state_.envelope_state[i] == EnvelopeGenerator::ATTACK)
        buf.writeByte(1);
      else if (state_.envelope_state[i] == EnvelopeGenerator::DECAY_SUSTAIN)
        buf.writeByte(2);
      else
        buf.writeByte(0);
      buf.writeBoolean(state_.hold_zero[i]);
    }
  }

  void SID::saveState(Plus4Emu::File& f)
  {
    Plus4Emu::File::Buffer  buf;
    this->saveState(buf);
    f.addChunk(Plus4Emu::File::PLUS4EMU_CHUNKTYPE_SID_STATE, buf);
  }

  void SID::loadState(Plus4Emu::File::Buffer& buf)
  {
    buf.setPosition(0);
    // check version number
    unsigned int  version = buf.readUInt32();
    // TODO: update snapshot format for new reSID version
    if (version != 0x01000000) {
      buf.setPosition(buf.getDataSize());
      throw Plus4Emu::Exception("incompatible SID snapshot format");
    }
    try {
      // load saved state
      State   state_;
      for (uint8_t i = 0x00; i <= 0x1F; i++)
        state_.sid_register[i] = char(buf.readByte());
      state_.bus_value = buf.readByte();
      state_.bus_value_ttl = buf.readInt32();
      for (uint8_t i = 0; i < 3; i++) {
        state_.accumulator[i] = buf.readUInt32() & 0x00FFFFFFU;
        state_.shift_register[i] = buf.readUInt32() & 0x00FFFFFFU;
        state_.rate_counter[i] = buf.readUInt32() & 0x0000FFFFU;
        state_.rate_counter_period[i] = buf.readUInt32() & 0x0000FFFFU;
        state_.exponential_counter[i] = buf.readUInt32() & 0x0000FFFFU;
        state_.exponential_counter_period[i] = buf.readUInt32() & 0x0000FFFFU;
        state_.envelope_counter[i] = buf.readByte();
        uint8_t tmp = buf.readByte();
        if (tmp == 1)
          state_.envelope_state[i] = EnvelopeGenerator::ATTACK;
        else if (tmp == 2)
          state_.envelope_state[i] = EnvelopeGenerator::DECAY_SUSTAIN;
        else
          state_.envelope_state[i] = EnvelopeGenerator::RELEASE;
        state_.hold_zero[i] = buf.readBoolean();
      }
      if (buf.getPosition() != buf.getDataSize())
        throw Plus4Emu::Exception("trailing garbage at end of "
                                  "SID snapshot data");
      write_state(state_);
    }
    catch (...) {
      // reset SID
      try {
        this->reset();
      }
      catch (...) {
      }
      throw;
    }
  }

  void SID::registerChunkType(Plus4Emu::File& f)
  {
    ChunkType_SIDSnapshot   *p;
    p = new ChunkType_SIDSnapshot(*this);
    try {
      f.registerChunkType(p);
    }
    catch (...) {
      delete p;
      throw;
    }
  }

}       // namespace Plus4

