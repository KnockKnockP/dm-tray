#pragma once

#include <windows.h>
#include <cstdint>
#include <vector>

struct HImageFrame
{
	HBITMAP Bitmap = NULL;
	int FrameTime = 0;

	~HImageFrame();
};

struct HImage
{
	std::vector<HImageFrame> Frames;
	int Width = 0;
	int Height = 0;

	HImage();
	~HImage();
	HImage(const HImage&) = delete;
	HImage(HImage&&) = default;

	bool IsValid() const noexcept {
		return !Frames.empty();
	}

	// Withdraws a bitmap handle based on frame number.  This removes it from
	// the management of HImage, so deleting the HImage instance won't delete
	// the returned bitmap.
	HBITMAP WithdrawImage(size_t frameNo, int* frameLengthOut);

	// Gets a bitmap based on frame number.  This doesn't remove it from the
	// management of HImage. As such, deleting the HImage instance won't delete
	// the returned image.
	HBITMAP GetImage(size_t frameNo, int* frameLengthOut) const;

	// Gets the first frame of an image.
	// Use when you have no intention of animating the image.
	HBITMAP GetFirstFrame() const;
};

class ImageLoader
{
public:
	// outHasAlphaChannel - If HBITMAP is valid, this value means whether or not the alpha channel is valid.  If
	// false, you shouldn't use AlphaBlend because the channel is most likely filled with zero, which is bad.
	//
	// loadAllFrames - If false, loads only the first frame.
	static HImage* ConvertToBitmap(const uint8_t* pData, size_t size, bool& outHasAlphaChannel, bool loadAllFrames = true, int width = 0, int height = 0);

	static HBITMAP LoadFromFile(const char* pFileName, bool& outHasAlphaChannel);

	static bool ConvertToPNG(std::vector<uint8_t>* outData, void* pBits, int width, int height, int widthBytes, int bpp, bool forceOpaque, bool flipVerticallyWhenSaving);
};
