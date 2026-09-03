/**
 * @file
 * @brief What a file says about the track it holds.
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

#include <fileref.h>
#include <tag.h>
#include <tpropertymap.h>
#include <tstringlist.h>
#include <tvariant.h>

#include <audioproperties.h>

#include <chrono>

namespace wiola::codec {

namespace {

/// A tag's text as this program keeps text, which is UTF-8 whatever the file wrote it in.
[[nodiscard]] std::string as_utf8(const TagLib::String& text)
{
    return text.isEmpty() ? std::string{} : text.to8Bit(true);
}

/// The first picture a file carries, whichever kind of file it is. Asked for by name rather than
/// by format, so an MP3's `APIC` and a FLAC's picture block are read the same way.
void read_picture(Tags& tags, const TagLib::FileRef& file)
{
    const TagLib::List<TagLib::VariantMap> pictures{file.complexProperties("PICTURE")};

    if (pictures.isEmpty())
        return;

    const TagLib::VariantMap& picture{pictures.front()};
    const TagLib::ByteVector data{picture.value("data").toByteVector()};

    if (data.isEmpty())
        return;

    const auto* bytes = reinterpret_cast<const std::byte*>(data.data());

    tags.art.assign(bytes, bytes + data.size());
    tags.art_type = as_utf8(picture.value("mimeType").toString());
}

} // namespace

Tags read_tags(const std::filesystem::path& path)
{
    // How long it runs is worth having and comes of the same open, but only as the header
    // states it: the reading that works it out for a file whose header does not is the reading
    // this is here to avoid.
    const TagLib::FileRef file{path.c_str(), true, TagLib::AudioProperties::Fast};

    if (file.isNull() || file.tag() == nullptr)
        return {};

    Tags tags;

    tags.title = as_utf8(file.tag()->title());
    tags.artist = as_utf8(file.tag()->artist());
    tags.album = as_utf8(file.tag()->album());

    if (const TagLib::AudioProperties* properties = file.audioProperties())
        tags.duration = units::Time{std::chrono::milliseconds{properties->lengthInMilliseconds()}};

    read_picture(tags, file);

    return tags;
}

} // namespace wiola::codec
