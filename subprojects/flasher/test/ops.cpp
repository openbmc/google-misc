#include <flasher/file/memory.hpp>
#include <flasher/device/fake.hpp>
#include <flasher/device/mock.hpp>
#include <flasher/mutate.hpp>
#include <flasher/ops.hpp>
#include <gtest/gtest.h>

#include <stdexcept>
#include <optional>
#include <memory>
#include <vector>

namespace flasher
{
namespace ops
{

using testing::_;
using testing::Return;

void fillBuffer(std::vector<std::byte>& buf, size_t off, size_t size)
{
	buf.resize(size);
	for (size_t i = 0; i < size; ++i)
	{
		buf[i] = static_cast<std::byte>(off + i);
	}
}

class AsymmetricMutate : public Mutate
{
  public:
    void forward(stdplus::span<std::byte> data, size_t offset) override
    {
        for (size_t i = 0; i < data.size(); ++i)
	{
            reinterpret_cast<uint8_t&>(data[i]) += i + offset;
	}
    }

    void reverse(stdplus::span<std::byte> data, size_t offset) override
    {
        for (size_t i = 0; i < data.size(); ++i)
	{
            reinterpret_cast<uint8_t&>(data[i]) -= i + offset;
	}
    }
};

class VerifyTest : public testing::Test
{
  protected:
    VerifyTest() : d(df, device::Fake::Type::Simple, 0) {
    fillBuffer(df.data, 0, 12);
    fillBuffer(f.data, 0, 12);
    d = device::Fake(df, device::Fake::Type::Nor, 4);
    }
    file::Memory df, f;
    device::Fake d;
    NestedMutate m;
};

TEST_F(VerifyTest, InvalidOffset)
{
    EXPECT_THROW(verify(d, /*dev_offset=*/13, f, /*file_offset=*/0, m, /*max_size=*/0, /*stride_size=*/std::nullopt), std::invalid_argument);
}

TEST_F(VerifyTest, InvalidStride)
{
    EXPECT_THROW(verify(d, /*dev_offset=*/0, f, /*file_offset=*/0, m, /*max_size=*/0, /*stride_size=*/0), std::invalid_argument);
}

TEST_F(VerifyTest, ValidWhole)
{
    verify(d, /*dev_offset=*/0, f, /*file_offset=*/0, m, /*max_size=*/20, /*stride_size=*/std::nullopt);
    verify(d, /*dev_offset=*/0, f, /*file_offset=*/0, m, /*max_size=*/12, /*stride_size=*/1);
    verify(d, /*dev_offset=*/0, f, /*file_offset=*/0, m, /*max_size=*/11, /*stride_size=*/std::nullopt);
}

TEST_F(VerifyTest, SmallFile)
{
    f.data.resize(4);
    verify(d, /*dev_offset=*/0, f, /*file_offset=*/0, m, /*max_size=*/20, /*stride_size=*/std::nullopt);
}

TEST_F(VerifyTest, FileTooBig)
{
    fillBuffer(f.data, 0, 13);
    EXPECT_THROW(verify(d, /*dev_offset=*/0, f, /*file_offset=*/0, m, /*max_size=*/20, /*stride_size=*/std::nullopt), std::runtime_error);
}

TEST_F(VerifyTest, ValidSubset)
{
    fillBuffer(f.data, 2, 10);
    verify(d, /*dev_offset=*/3, f, /*file_offset=*/1, m, /*max_size=*/20, /*stride_size=*/std::nullopt);
}


TEST_F(VerifyTest, TooBigSubset)
{
    fillBuffer(f.data, 2, 11);
    EXPECT_THROW(verify(d, /*dev_offset=*/3, f, /*file_offset=*/1, m, /*max_size=*/20, /*stride_size=*/std::nullopt), std::runtime_error);
}

TEST_F(VerifyTest, RespectSize)
{
    fillBuffer(f.data, 2, 11);
    f.data[5] = std::byte{0xff};
    verify(d, /*dev_offset=*/3, f, /*file_offset=*/1, m, /*max_size=*/4, /*stride_size=*/std::nullopt);
}

TEST_F(VerifyTest, MutateApplied)
{
    m.mutations.push_back(std::make_unique<AsymmetricMutate>());
    m.forward(df.data, 0);
    verify(d, /*dev_offset=*/1, f, /*file_offset=*/1, m, /*max_size=*/4, /*stride_size=*/std::nullopt);
}

class EraseTest : public testing::Test
{
  protected:
    EraseTest() : d(Device::Type::Nor, 12, 4) {}
    testing::StrictMock<device::Mock> d;
};

TEST_F(EraseTest, ArgumentValidation)
{
  EXPECT_THROW(erase(d, /*dev_offset=*/0, /*max_size=*/0, /*stride_size=*/0, /*noread=*/false), std::invalid_argument);
  EXPECT_THROW(erase(d, /*dev_offset=*/0, /*max_size=*/0, /*stride_size=*/2, /*noread=*/false), std::invalid_argument);
  EXPECT_THROW(erase(d, /*dev_offset=*/16, /*max_size=*/0, /*stride_size=*/4, /*noread=*/false), std::invalid_argument);
  EXPECT_THROW(erase(d, /*dev_offset=*/1, /*max_size=*/0, /*stride_size=*/4, /*noread=*/false), std::invalid_argument);
  EXPECT_THROW(erase(d, /*dev_offset=*/0, /*max_size=*/11, /*stride_size=*/4, /*noread=*/false), std::invalid_argument);
  erase(d, /*dev_offset=*/0, /*max_size=*/0, /*stride_size=*/4, /*noread=*/false);

  EXPECT_CALL(d,recommendedStride()).WillOnce(Return(0));
  EXPECT_THROW(erase(d, /*dev_offset=*/0, /*max_size=*/0, /*stride_size=*/std::nullopt, /*noread=*/false), std::invalid_argument);
}

TEST_F(EraseTest, NoRead)
{
  EXPECT_CALL(d, eraseBlocks(1,2));
  erase(d, /*dev_offset=*/4, /*max_size=*/9, /*stride_size=*/8, /*noread=*/true);
}

TEST_F(EraseTest, WithRead)
{
  testing::InSequence seq;
  EXPECT_CALL(d, readAt(_, 0)).WillOnce([](stdplus::span<std::byte> buf, size_t) { memset(buf.data(), 0xff, buf.size()); return buf; });
  EXPECT_CALL(d, readAt(_, 4)).WillOnce([](stdplus::span<std::byte> buf, size_t) { memset(buf.data(), 0, buf.size()); return buf; });
  EXPECT_CALL(d, eraseBlocks(1,1));
  erase(d, /*dev_offset=*/0, /*max_size=*/8, /*stride_size=*/4, /*noread=*/false);
}

}
} // namespace flasher
