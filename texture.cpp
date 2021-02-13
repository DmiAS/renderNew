﻿#include "texture.h"
#include "bary.h"
#include <QtDebug>

TextureShader::TextureShader(const std::string& fileName){
    texture.load(QString::fromStdString(fileName));
}

Vec3f TextureShader::shade(const Vertex &a, const Vertex &b, const Vertex &c, const Vec3f &bary){
    Vec3f pixel = baryCentricInterpolation(a.pos, b.pos, c.pos, bary);
//    float pixel_u = interPolateCord(a.u / a.pos.z , b.u / b.pos.z, c.u / c.pos.z, bary) * pixel.z;
//    float pixel_v = interPolateCord(a.v / a.pos.z, b.v / b.pos.z, c.v / c.pos.z, bary) * pixel.z;
    float invZ = 1 / pixel.z;
    float pixel_u = interPolateCord(a.u , b.u, c.u, bary) * invZ ;
    float pixel_v = interPolateCord(a.v, b.v, c.v, bary) * invZ;

    int x = std::floor(pixel_u * (texture.width() - 1) );
    int y = std::floor(pixel_v * (texture.height() - 1));

    auto color = texture.pixelColor(x, y);
    Vec3f pixel_color = Vec3f{(float)color.red(), (float)color.green(), (float)color.blue()};

    return pixel_color;
}
