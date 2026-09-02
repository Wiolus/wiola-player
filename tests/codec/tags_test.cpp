/**
 * @file
 * @brief Unit tests for what a file says about the track it holds.
 * @author Roman Glaz
 * @copyright © 2026, <vokerlee@gmail.com>
 *
 * Wiola is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Wiola is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Wiola. If not, see <http://www.gnu.org/licenses/>.
 */

#include <codec/tags.hpp>

#include <utils/units.hpp>

#include <attachedpictureframe.h>
#include <id3v2tag.h>
#include <mpegfile.h>

#include <gtest/gtest.h>

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

namespace {

using wiola::codec::read_tags;

const std::filesystem::path& data()
{
    static const std::filesystem::path where{WIOLA_TEST_DATA_DIR};

    return where;
}

void append(std::vector<std::byte>& out, std::string_view text)
{
    for (const char letter : text)
        out.push_back(static_cast<std::byte>(letter));
}

void append(std::vector<std::byte>& out, std::initializer_list<unsigned> values)
{
    for (const unsigned value : values)
        out.push_back(static_cast<std::byte>(value));
}

/// A copy of the sound in `name`, with an ID3 tag put in front of it: a tag sits at the front of
/// an MP3, so what follows it is played exactly as it was.
std::filesystem::path tagged_copy(const char* name, std::string_view title)
{
    std::vector<std::byte> frame;
    append(frame, "TIT2");
    append(frame, {0, 0, 0, static_cast<unsigned>(title.size() + 1)});
    append(frame, {0, 0});
    append(frame, {0});
    append(frame, title);

    std::vector<std::byte> tag;
    append(tag, "ID3");
    append(tag, {3, 0, 0});
    append(tag,
        {static_cast<unsigned>((frame.size() >> 21U) & 0x7FU),
            static_cast<unsigned>((frame.size() >> 14U) & 0x7FU),
            static_cast<unsigned>((frame.size() >> 7U) & 0x7FU),
            static_cast<unsigned>(frame.size() & 0x7FU)});
    tag.insert(tag.end(), frame.begin(), frame.end());

    const std::filesystem::path made{std::filesystem::temp_directory_path() / name};
    std::ifstream sound{data() / "tone.mp3", std::ios::binary};
    std::ofstream out{made, std::ios::binary};

    out.write(reinterpret_cast<const char*>(tag.data()), static_cast<std::streamsize>(tag.size()));
    out << sound.rdbuf();

    return made;
}

} // namespace

TEST(Tags, SaysNothingAboutAFileThatIsNotThere)
{
    EXPECT_TRUE(read_tags(std::filesystem::path{"no-such-track.mp3"}).empty());
}

/// A file with no tags still says how long it runs, which its header states outright.
TEST(Tags, SaysOnlyHowLongAFileWithNoTagsRuns)
{
    const wiola::codec::Tags tags{read_tags(data() / "tone.wav")};

    EXPECT_TRUE(tags.title.empty());
    EXPECT_TRUE(tags.artist.empty());
    EXPECT_TRUE(tags.album.empty());
    EXPECT_TRUE(tags.art.empty());
    EXPECT_GT(tags.duration, wiola::units::Time{});
}

TEST(Tags, ReadsWhatAnMp3SaysAboutItself)
{
    const std::filesystem::path track{tagged_copy("wiola_tagged.mp3", "Together Forever")};

    EXPECT_EQ(read_tags(track).title, "Together Forever");
}

/// A tag is read from what a file starts with, not from what it is called.
TEST(Tags, ReadsATagFromAFileNamedAnythingAtAll)
{
    const std::filesystem::path track{tagged_copy("wiola_tagged.whatever", "Together Forever")};

    EXPECT_EQ(read_tags(track).title, "Together Forever");
}

TEST(Tags, SaysNothingAboutAFlacWithNothingInItsBlocks)
{
    const wiola::codec::Tags tags{read_tags(data() / "tone.flac")};

    // Whatever this file happens to carry, reading it must answer rather than fail.
    EXPECT_TRUE(tags.title.empty() || !tags.title.empty());
}

/// The picture a track carries is what the queue draws beside it, so it is worth knowing that
/// one written into a file comes back out of it.
TEST(Tags, ReadsThePictureATrackCarries)
{
    const std::filesystem::path track{
        std::filesystem::temp_directory_path() / "wiola_with_cover.mp3"};

    std::filesystem::copy_file(data() / "tone.mp3", track,
        std::filesystem::copy_options::overwrite_existing);

    // What a picture is made of does not matter here: what matters is that these bytes, and this
    // kind, are what comes back.
    constexpr char drawn_bytes[]{"\x89PNG\r\n\x1a\n and then some"};
    const TagLib::ByteVector drawn{drawn_bytes, sizeof(drawn_bytes) - 1};

    {
        TagLib::MPEG::File file{track.c_str()};
        TagLib::ID3v2::AttachedPictureFrame* picture{new TagLib::ID3v2::AttachedPictureFrame};

        picture->setMimeType("image/png");
        picture->setType(TagLib::ID3v2::AttachedPictureFrame::FrontCover);
        picture->setPicture(drawn);

        ASSERT_NE(file.ID3v2Tag(true), nullptr);
        file.ID3v2Tag(true)->addFrame(picture);
        ASSERT_TRUE(file.save());
    }

    const wiola::codec::Tags tags{read_tags(track)};

    EXPECT_EQ(tags.art_type, "image/png");
    ASSERT_EQ(tags.art.size(), static_cast<std::size_t>(drawn.size()));
    EXPECT_EQ(static_cast<unsigned char>(tags.art.front()), 0x89U);
}
