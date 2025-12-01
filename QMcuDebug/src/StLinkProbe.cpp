#include <QMcu/Debug/StLinkProbe.hpp>

#include <QTimer>

#include <stlink.h>

StLinkProbe* StLinkProbe::instance_ = nullptr;

StLinkProbe::StLinkProbe(QObject* parent) : QObject(parent)
{
  if(instance_ != nullptr)
  {
    throw std::runtime_error("StLinkProbe is a singleton");
  }

  instance_ = this;

  QTimer::singleShot(
      0,
      [this]
      {
        sl_ = stlink_open_usb(UINFO, CONNECT_HOT_PLUG, serial_.toUtf8().data(), speed_);
        if(sl_ == nullptr)
        {
          throw std::runtime_error{"STM32 target not found"};
        }

        qDebug() << "chip-id:" << sl_->chip_id << ", core-id:" << sl_->core_id;

        if(stlink_current_mode(sl_) != STLINK_DEV_DEBUG_MODE && stlink_enter_swd_mode(sl_))
        {
          throw std::runtime_error{"cannot enter SWD mode"};
        }

        stlink_status(sl_);
      });
}

StLinkProbe::~StLinkProbe()
{
  if(sl_ != nullptr)
  {
    stlink_close(sl_);
  }
}

constexpr bool is_aligned(std::integral auto addr, std::size_t alignment) noexcept
{
  return (addr & (alignment - 1)) == 0;
}

constexpr auto align_up(std::integral auto addr, std::size_t alignment) noexcept
{
  return (addr + alignment - 1) & ~(alignment - 1);
}

constexpr auto align_down(std::integral auto addr, std::size_t alignment) noexcept
{
  return addr & ~(alignment - 1);
}

bool StLinkProbe::read(address_t address, std::span<std::byte> data)
{
  static constexpr auto expected_alignment = sizeof(uint32_t);
  size_t                buffer_offset      = 0;

  if(not is_aligned(address, expected_alignment))
  {
    // read until address
    const auto   down          = align_down(address, expected_alignment);
    const size_t bytes_to_drop = address - down;
    const size_t bytes_to_read =
        std::max(expected_alignment,
                 std::min(expected_alignment - bytes_to_drop, data.size_bytes()));

    uint32_t   value = 0;
    const auto res   = sl_->backend->read_debug32(sl_, down, &value);

    std::memcpy(data.data(),
                reinterpret_cast<uint8_t*>(&value) + bytes_to_drop,
                std::min(bytes_to_read, data.size_bytes()));
    buffer_offset += bytes_to_read;
    if(buffer_offset > data.size_bytes())
    {
      return true;
    }
  }

  assert(is_aligned(address + buffer_offset, expected_alignment));

  for(; buffer_offset < data.size_bytes(); buffer_offset += expected_alignment)
  {
    const uint32_t addr  = address + buffer_offset;
    uint32_t       value = 0;
    const auto     res   = sl_->backend->read_debug32(sl_, addr, &value);

    const size_t bytes_to_read = std::min(expected_alignment, data.size_bytes() - buffer_offset);
    std::memcpy(data.data() + buffer_offset, &value, bytes_to_read);
  }

  return true;
}
