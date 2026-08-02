#pragma once

#include "Precompiled.h"
#include "Renderer2D.h"

class WeatherRenderer
{
public:
    WeatherRenderer(const Renderer2D& renderer);
    ~WeatherRenderer();

public:
    void DrawDefault();
    void DrawClouds();

    void DrawHarvest();
    void DrawSunset();

private:
    Renderer2D& m_renderer;
};