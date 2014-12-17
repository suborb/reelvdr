
void MyDrawImage(unsigned int imageId, int x, int y, bool blend, int horRepeat, int vertRepeat)
{
    CachedImage const *img = (CachedImage const *)hda->osd_cached_images[imageId];
    int m, n, h, v;
    int width;
    int height;
    unsigned int const *srcPixels;

    if (img)
    {
        width = img->width;
        height = img->height;

        if (blend) {
                for (v = vertRepeat; v > 0; --v)
                {
                        srcPixels = (unsigned int const *)(img + 1);
                        for (n = height; n > 0; --n)
                        {
                                unsigned int *tgtPixels = (unsigned int*)(osd->buffer + osd->bpp * OSD_WIDTH * y++ + x*osd->bpp);
                                for (h = horRepeat; h > 0; --h)
                                {
                                        unsigned int const *src = srcPixels;
                                        for (m = width; m > 0; --m)
                                        {
                                                  *tgtPixels = AlphaBlend((*src++), (*tgtPixels) );
                                                ++ tgtPixels;
                                        }
                                }
                                srcPixels += width;
                        }
                }
        }
        else {
                for (v = vertRepeat; v > 0; --v)
                {
                        srcPixels = (unsigned int const *)(img + 1);
                        for (n = height; n > 0; --n)
                        {
                                unsigned int *tgtPixels = (unsigned int*)(osd->buffer + osd->bpp * OSD_WIDTH * y++ + x*osd->bpp);
                                for (h = horRepeat; h > 0; --h)
                                {
                                        unsigned int const *src = srcPixels;
                                        memcpy(tgtPixels, src, width*sizeof(int));
                                        tgtPixels+=width;
                                        src+=width;
                                }
                                srcPixels += width;
                        }
                }
        }

    }
    else
    {
        printf("Image %d not cached.\n", imageId);
    }
}


static unsigned int AlphaBlend(unsigned int p2, unsigned int p1)
{

    unsigned int const xa1 =  (p1 >> 24);
    unsigned int const xa2 = (p2 >> 24);

//    printf("blend: fg: %#x bg: %#x\n", p2, p1);

    if (xa2==255 || !xa1)
    {
        return p2;
    }
    else if (!xa2)
    {
        return p1;
    }
    else
    {
        unsigned int const a1 = 255 - xa1;
        unsigned int const a2 = 255 - xa2;

        unsigned int const r2 = (p2 >> 16) & 255;
        unsigned int const g2 = (p2 >> 8) & 255;
        unsigned int const b2 = p2  & 255;

        unsigned int const r1 = (p1 >> 16) & 255;
        unsigned int const g1 = (p1 >> 8) & 255;
        unsigned int const b1 = p1 & 255;

        unsigned int const al1_al2 = a1 * a2;
        unsigned int const al2     = a2 * 255;
        unsigned int const one     = 255 * 255;

        unsigned int const al1_al2_inv = one - al1_al2; // > 0

        unsigned int const c1 = al2 - al1_al2;
        unsigned int const c2 = one - al2;

        unsigned int const a = al1_al2 / 255;
        unsigned int const r = (c1 * r1 + c2 * r2) / al1_al2_inv;
        unsigned int const g = (c1 * g1 + c2 * g2) / al1_al2_inv;
        unsigned int const b = (c1 * b1 + c2 * b2) / al1_al2_inv;
        unsigned int res = ((255 - a) << 24) | (r << 16) | (g << 8) | b;
        return res;
    }
}

