#include "pch.h"
#include "SimpleCapture.h"
#include "ScreenRecordingToolProvider.h"

namespace winrt
{
    using namespace Windows::Foundation;
    using namespace Windows::Foundation::Numerics;
    using namespace Windows::Graphics;
    using namespace Windows::Graphics::Capture;
    using namespace Windows::Graphics::DirectX;
    using namespace Windows::Graphics::DirectX::Direct3D11;
    using namespace Windows::System;
    using namespace Windows::UI;
    using namespace Windows::UI::Composition;
}

namespace util
{
    using namespace robmikh::common::uwp;
}

SimpleCapture::SimpleCapture(winrt::IDirect3DDevice const& device, winrt::GraphicsCaptureItem const& item, int framerate, int framesBufferSize)
{
    m_item = item;
    m_device = device;
    m_fileFormatGuid = winrt::BitmapEncoder::JpegEncoderId();
    m_bitmapPixelFormat = winrt::BitmapPixelFormat::Bgra8;
    auto pixelFormat = winrt::DirectXPixelFormat::B8G8R8A8UIntNormalized;

    m_d3dDevice = GetDXGIInterfaceFromObject<ID3D11Device>(m_device);
    m_d3dDevice->GetImmediateContext(m_d3dContext.put());

    // Creating our frame pool with 'Create' instead of 'CreateFreeThreaded'
    // means that the frame pool's FrameArrived event is called on the thread
    // the frame pool was created on. This also means that the creating thread
    // must have a DispatcherQueue. If you use this method, it's best not to do
    // it on the UI thread. 
    m_framePool = winrt::Direct3D11CaptureFramePool::CreateFreeThreaded(m_device, pixelFormat, 2, m_item.Size());
    m_session = m_framePool.CreateCaptureSession(m_item);
    m_lastSize = m_item.Size();
    m_framePool.FrameArrived({ this, &SimpleCapture::OnFrameArrived });

    m_frameInterval = 1000 / framerate;
    m_framesBufferSize = framesBufferSize;
}

void SimpleCapture::StartCapture()
{
    CheckClosed();
    m_lastFrameTime = std::chrono::steady_clock::now();
    m_session.StartCapture();
}

void SimpleCapture::Close()
{
    auto expected = false;
    if (m_closed.compare_exchange_strong(expected, true))
    {
        m_session.Close();
        m_framePool.Close();

        m_framePool = nullptr;
        m_session = nullptr;
        m_item = nullptr;
    }
}

void SimpleCapture::CloseAndSave(StorageFolder storageFolder)
{
    auto expected = false;
    if (m_closed.compare_exchange_strong(expected, true))
    {

        m_session.Close();
        m_framePool.Close();

        SaveFrames(storageFolder);

        m_framePool = nullptr;
        m_session = nullptr;
        m_item = nullptr;
    }
}

void SimpleCapture::OnFrameArrived(winrt::Direct3D11CaptureFramePool const& sender, winrt::IInspectable const&)
{
    ReceivedFrameEvent();
    auto frame = sender.TryGetNextFrame();

    auto now = std::chrono::steady_clock::now();
    auto timeSinceLastFrame = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastFrameTime).count();
    
    if (timeSinceLastFrame >= m_frameInterval) 
    {
        // Store frame

        auto surfaceTexture = GetDXGIInterfaceFromObject<ID3D11Texture2D>(frame.Surface());

        D3D11_TEXTURE2D_DESC desc{};
        surfaceTexture->GetDesc(&desc);

        winrt::com_ptr<ID3D11Texture2D> frameTexture;
        winrt::check_hresult(m_d3dDevice->CreateTexture2D(&desc, nullptr, frameTexture.put()));

        m_d3dContext->CopyResource(frameTexture.get(), surfaceTexture.get());

        // Check if m_frames has reached maximum size
        if (m_frames.size() == m_framesBufferSize) 
        {
            // Remove oldest frame
            m_frames.erase(m_frames.begin());
        }

        m_frames.push_back(frameTexture);

        StoredFrameEvent();

        m_lastFrameTime = now;
    }
}

void SimpleCapture::SaveFrames(StorageFolder storageFolder)
{
    int i = 1;
    for (const auto& frame : m_frames) {
        std::wstringstream wss;
        wss << L"screenshot" << std::setw(4) << std::setfill(L'0') << i << L".jpg";
        std::wstring filename = wss.str();
        Windows::Storage::StorageFile file = storageFolder.CreateFileAsync(filename.c_str(), Windows::Storage::CreationCollisionOption::ReplaceExisting).get();

        // Get the file stream
        auto stream = file.OpenAsync(winrt::FileAccessMode::ReadWrite).get();

        // Initialize the encoder
        auto encoder = winrt::BitmapEncoder::CreateAsync(m_fileFormatGuid, stream).get();

        // Encode the image
        D3D11_TEXTURE2D_DESC desc = {};
        frame->GetDesc(&desc);
        auto bytes = util::CopyBytesFromTexture(frame);
        encoder.SetPixelData(
            m_bitmapPixelFormat,
            winrt::BitmapAlphaMode::Premultiplied,
            desc.Width,
            desc.Height,
            1.0,
            1.0,
            bytes);
        encoder.FlushAsync().get();

        i++;
    }
}
