#include "pch.h"
#include "CircularFrameBuffer.h"

CircularFrameBuffer::CircularFrameBuffer(size_t capacity, bool asMegabytes) : m_capacity(capacity), m_asMegabytes(asMegabytes), m_memoryUsage(0)
{
}

void CircularFrameBuffer::add_frame(winrt::com_ptr<ID3D11Texture2D> texture, const std::string& filename) 
{
    size_t frame_size = calculate_frame_size(texture);

    if (m_asMegabytes) 
    {
        while (m_memoryUsage + frame_size > m_capacity && !m_frames.empty()) 
        {
            m_memoryUsage -= m_frames.front().size;
            m_frames.pop_front();
        }
    }
    else if (m_frames.size() == m_capacity)
    {
        m_memoryUsage -= m_frames.front().size;
        m_frames.pop_front();
    }

    m_frames.push_back({ texture, filename, frame_size });
    m_memoryUsage += frame_size;
}

size_t CircularFrameBuffer::calculate_frame_size(winrt::com_ptr<ID3D11Texture2D> texture) 
{
    D3D11_TEXTURE2D_DESC desc;
    texture->GetDesc(&desc);
    size_t row_pitch = (desc.Width * desc.Format * 8 + 7) / 8;
    size_t slice_pitch = row_pitch * desc.Height;
    return slice_pitch * desc.ArraySize;
}

void CircularFrameBuffer::save_frames(winrt::Windows::Storage::StorageFolder storageFolder) 
{
    for (const auto& frame : m_frames) 
    {
        auto file = storageFolder.CreateFileAsync(winrt::to_hstring(frame.filename), winrt::Windows::Storage::CreationCollisionOption::ReplaceExisting).get();

        // Get the file stream
        auto stream = file.OpenAsync(winrt::Windows::Storage::FileAccessMode::ReadWrite).get();

        // Initialize the encoder
        auto encoder = winrt::Windows::Graphics::Imaging::BitmapEncoder::CreateAsync(winrt::Windows::Graphics::Imaging::BitmapEncoder::JpegEncoderId(), stream).get();

        // Encode the image
        D3D11_TEXTURE2D_DESC desc = {};
        frame.texture->GetDesc(&desc);
        auto bytes = util::CopyBytesFromTexture(frame.texture);
        encoder.SetPixelData(
            winrt::Windows::Graphics::Imaging::BitmapPixelFormat::Bgra8,
            winrt::Windows::Graphics::Imaging::BitmapAlphaMode::Premultiplied,
            desc.Width,
            desc.Height,
            1.0,
            1.0,
            bytes);
        encoder.FlushAsync().get();
    }
}
