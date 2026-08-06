#ifndef CY_PIXEL_FORMAT_H
#define CY_PIXEL_FORMAT_H

typedef enum _CY_PIXEL_FORMAT_
{
    CY_PIXEL_FORMAT_Mono1p                              = 0x01010037, /* Monochrome 1-bit packed */
    CY_PIXEL_FORMAT_Mono2p                              = 0x01020038, /* Monochrome 2-bit packed */
    CY_PIXEL_FORMAT_Mono4p                              = 0x01040039, /* Monochrome 4-bit packed */
    CY_PIXEL_FORMAT_Mono8                               = 0x01080001, /* Monochrome 8-bit */
    CY_PIXEL_FORMAT_Mono8s                              = 0x01080002, /* Monochrome 8-bit signed */
    CY_PIXEL_FORMAT_Mono10                              = 0x01100003, /* Monochrome 10-bit unpacked */
    CY_PIXEL_FORMAT_Mono10p                             = 0x010A0046, /* Monochrome 10-bit packed */
    CY_PIXEL_FORMAT_Mono12                              = 0x01100005, /* Monochrome 12-bit unpacked */
    CY_PIXEL_FORMAT_Mono12p                             = 0x010C0047, /* Monochrome 12-bit packed */
    CY_PIXEL_FORMAT_Mono14                              = 0x01100025, /* Monochrome 14-bit unpacked */
    CY_PIXEL_FORMAT_Mono14p                             = 0x010E0104, /* Monochrome 14-bit packed */
    CY_PIXEL_FORMAT_Mono16                              = 0x01100007, /* Monochrome 16-bit */
    CY_PIXEL_FORMAT_Mono32                              = 0x01200111, /* Monochrome 32-bit */
    CY_PIXEL_FORMAT_BayerBG4p                           = 0x01040110, /* Bayer Blue-Green 4-bit packed */
    CY_PIXEL_FORMAT_BayerBG8                            = 0x0108000B, /* Bayer Blue-Green 8-bit */
    CY_PIXEL_FORMAT_BayerBG10                           = 0x0110000F, /* Bayer Blue-Green 10-bit unpacked */
    CY_PIXEL_FORMAT_BayerBG10p                          = 0x010A0052, /* Bayer Blue-Green 10-bit packed */
    CY_PIXEL_FORMAT_BayerBG12                           = 0x01100013, /* Bayer Blue-Green 12-bit unpacked */
    CY_PIXEL_FORMAT_BayerBG12p                          = 0x010C0053, /* Bayer Blue-Green 12-bit packed */
    CY_PIXEL_FORMAT_BayerBG14                           = 0x0110010C, /* Bayer Blue-Green 14-bit */
    CY_PIXEL_FORMAT_BayerBG14p                          = 0x010E0108, /* Bayer Blue-Green 14-bit packed */
    CY_PIXEL_FORMAT_BayerBG16                           = 0x01100031, /* Bayer Blue-Green 16-bit */
    CY_PIXEL_FORMAT_BayerGB4p                           = 0x0104010F, /* Bayer Green-Blue 4-bit packed */
    CY_PIXEL_FORMAT_BayerGB8                            = 0x0108000A, /* Bayer Green-Blue 8-bit */
    CY_PIXEL_FORMAT_BayerGB10                           = 0x0110000E, /* Bayer Green-Blue 10-bit unpacked */
    CY_PIXEL_FORMAT_BayerGB10p                          = 0x010A0054, /* Bayer Green-Blue 10-bit packed */
    CY_PIXEL_FORMAT_BayerGB12                           = 0x01100012, /* Bayer Green-Blue 12-bit unpacked */
    CY_PIXEL_FORMAT_BayerGB12p                          = 0x010C0055, /* Bayer Green-Blue 12-bit packed */
    CY_PIXEL_FORMAT_BayerGB14                           = 0x0110010B, /* Bayer Green-Blue 14-bit */
    CY_PIXEL_FORMAT_BayerGB14p                          = 0x010E0107, /* Bayer Green-Blue 14-bit packed */
    CY_PIXEL_FORMAT_BayerGB16                           = 0x01100030, /* Bayer Green-Blue 16-bit */
    CY_PIXEL_FORMAT_BayerGR4p                           = 0x0104010D, /* Bayer Green-Red 4-bit packed */
    CY_PIXEL_FORMAT_BayerGR8                            = 0x01080008, /* Bayer Green-Red 8-bit */
    CY_PIXEL_FORMAT_BayerGR10                           = 0x0110000C, /* Bayer Green-Red 10-bit unpacked */
    CY_PIXEL_FORMAT_BayerGR10p                          = 0x010A0056, /* Bayer Green-Red 10-bit packed */
    CY_PIXEL_FORMAT_BayerGR12                           = 0x01100010, /* Bayer Green-Red 12-bit unpacked */
    CY_PIXEL_FORMAT_BayerGR12p                          = 0x010C0057, /* Bayer Green-Red 12-bit packed */
    CY_PIXEL_FORMAT_BayerGR14                           = 0x01100109, /* Bayer Green-Red 14-bit */
    CY_PIXEL_FORMAT_BayerGR14p                          = 0x010E0105, /* Bayer Green-Red 14-bit packed */
    CY_PIXEL_FORMAT_BayerGR16                           = 0x0110002E, /* Bayer Green-Red 16-bit */
    CY_PIXEL_FORMAT_BayerRG4p                           = 0x0104010E, /* Bayer Red-Green 4-bit packed */
    CY_PIXEL_FORMAT_BayerRG8                            = 0x01080009, /* Bayer Red-Green 8-bit */
    CY_PIXEL_FORMAT_BayerRG10                           = 0x0110000D, /* Bayer Red-Green 10-bit unpacked */
    CY_PIXEL_FORMAT_BayerRG10p                          = 0x010A0058, /* Bayer Red-Green 10-bit packed */
    CY_PIXEL_FORMAT_BayerRG12                           = 0x01100011, /* Bayer Red-Green 12-bit unpacked */
    CY_PIXEL_FORMAT_BayerRG12p                          = 0x010C0059, /* Bayer Red-Green 12-bit packed */
    CY_PIXEL_FORMAT_BayerRG14                           = 0x0110010A, /* Bayer Red-Green 14-bit */
    CY_PIXEL_FORMAT_BayerRG14p                          = 0x010E0106, /* Bayer Red-Green 14-bit packed */
    CY_PIXEL_FORMAT_BayerRG16                           = 0x0110002F, /* Bayer Red-Green 16-bit */
    CY_PIXEL_FORMAT_RGBa8                               = 0x02200016, /* Red-Green-Blue-alpha 8-bit */
    CY_PIXEL_FORMAT_RGBa10                              = 0x0240005F, /* Red-Green-Blue-alpha 10-bit unpacked */
    CY_PIXEL_FORMAT_RGBa10p                             = 0x02280060, /* Red-Green-Blue-alpha 10-bit packed */
    CY_PIXEL_FORMAT_RGBa12                              = 0x02400061, /* Red-Green-Blue-alpha 12-bit unpacked */
    CY_PIXEL_FORMAT_RGBa12p                             = 0x02300062, /* Red-Green-Blue-alpha 12-bit packed */
    CY_PIXEL_FORMAT_RGBa14                              = 0x02400063, /* Red-Green-Blue-alpha 14-bit unpacked */
    CY_PIXEL_FORMAT_RGBa16                              = 0x02400064, /* Red-Green-Blue-alpha 16-bit */
    CY_PIXEL_FORMAT_RGB8                                = 0x02180014, /* Red-Green-Blue 8-bit */
    CY_PIXEL_FORMAT_RGB8_Planar                         = 0x02180021, /* Red-Green-Blue 8-bit planar */
    CY_PIXEL_FORMAT_RGB10                               = 0x02300018, /* Red-Green-Blue 10-bit unpacked */
    CY_PIXEL_FORMAT_RGB10_Planar                        = 0x02300022, /* Red-Green-Blue 10-bit unpacked planar */
    CY_PIXEL_FORMAT_RGB10p                              = 0x021E005C, /* Red-Green-Blue 10-bit packed */
    CY_PIXEL_FORMAT_RGB10p32                            = 0x0220001D, /* Red-Green-Blue 10-bit packed into 32-bit */
    CY_PIXEL_FORMAT_RGB12                               = 0x0230001A, /* Red-Green-Blue 12-bit unpacked */
    CY_PIXEL_FORMAT_RGB12_Planar                        = 0x02300023, /* Red-Green-Blue 12-bit unpacked planar */
    CY_PIXEL_FORMAT_RGB12p                              = 0x0224005D, /* Red-Green-Blue 12-bit packed */
    CY_PIXEL_FORMAT_RGB14                               = 0x0230005E, /* Red-Green-Blue 14-bit unpacked */
    CY_PIXEL_FORMAT_RGB16                               = 0x02300033, /* Red-Green-Blue 16-bit */
    CY_PIXEL_FORMAT_RGB16_Planar                        = 0x02300024, /* Red-Green-Blue 16-bit planar */
    CY_PIXEL_FORMAT_RGB565p                             = 0x02100035, /* Red-Green-Blue 5/6/5-bit packed */
    CY_PIXEL_FORMAT_BGRa8                               = 0x02200017, /* Blue-Green-Red-alpha 8-bit */
    CY_PIXEL_FORMAT_BGRa10                              = 0x0240004C, /* Blue-Green-Red-alpha 10-bit unpacked */
    CY_PIXEL_FORMAT_BGRa10p                             = 0x0228004D, /* Blue-Green-Red-alpha 10-bit packed */
    CY_PIXEL_FORMAT_BGRa12                              = 0x0240004E, /* Blue-Green-Red-alpha 12-bit unpacked */
    CY_PIXEL_FORMAT_BGRa12p                             = 0x0230004F, /* Blue-Green-Red-alpha 12-bit packed */
    CY_PIXEL_FORMAT_BGRa14                              = 0x02400050, /* Blue-Green-Red-alpha 14-bit unpacked */
    CY_PIXEL_FORMAT_BGRa16                              = 0x02400051, /* Blue-Green-Red-alpha 16-bit */
    CY_PIXEL_FORMAT_BGR8                                = 0x02180015, /* Blue-Green-Red 8-bit */
    CY_PIXEL_FORMAT_BGR10                               = 0x02300019, /* Blue-Green-Red 10-bit unpacked */
    CY_PIXEL_FORMAT_BGR10p                              = 0x021E0048, /* Blue-Green-Red 10-bit packed */
    CY_PIXEL_FORMAT_BGR12                               = 0x0230001B, /* Blue-Green-Red 12-bit unpacked */
    CY_PIXEL_FORMAT_BGR12p                              = 0x02240049, /* Blue-Green-Red 12-bit packed */
    CY_PIXEL_FORMAT_BGR14                               = 0x0230004A, /* Blue-Green-Red 14-bit unpacked */
    CY_PIXEL_FORMAT_BGR16                               = 0x0230004B, /* Blue-Green-Red 16-bit */
    CY_PIXEL_FORMAT_BGR565p                             = 0x02100036, /* Blue-Green-Red 5/6/5-bit packed */
    CY_PIXEL_FORMAT_R8                                  = 0x010800C9, /* Red 8-bit */
    CY_PIXEL_FORMAT_R10                                 = 0x010A00CA, /* Red 10-bit */
    CY_PIXEL_FORMAT_R12                                 = 0x010C00CB, /* Red 12-bit */
    CY_PIXEL_FORMAT_R16                                 = 0x011000CC, /* Red 16-bit */
    CY_PIXEL_FORMAT_G8                                  = 0x010800CD, /* Green 8-bit */
    CY_PIXEL_FORMAT_G10                                 = 0x010A00CE, /* Green 10-bit */
    CY_PIXEL_FORMAT_G12                                 = 0x010C00CF, /* Green 12-bit */
    CY_PIXEL_FORMAT_G16                                 = 0x011000D0, /* Green 16-bit */
    CY_PIXEL_FORMAT_B8                                  = 0x010800D1, /* Blue 8-bit */
    CY_PIXEL_FORMAT_B10                                 = 0x010A00D2, /* Blue 10-bit */
    CY_PIXEL_FORMAT_B12                                 = 0x010C00D3, /* Blue 12-bit */
    CY_PIXEL_FORMAT_B16                                 = 0x011000D4, /* Blue 16-bit */
    CY_PIXEL_FORMAT_Coord3D_ABC8                        = 0x021800B2, /* 3D coordinate A-B-C 8-bit */
    CY_PIXEL_FORMAT_Coord3D_ABC8_Planar                 = 0x021800B3, /* 3D coordinate A-B-C 8-bit planar */
    CY_PIXEL_FORMAT_Coord3D_ABC10p                      = 0x021E00DB, /* 3D coordinate A-B-C 10-bit packed */
    CY_PIXEL_FORMAT_Coord3D_ABC10p_Planar               = 0x021E00DC, /* 3D coordinate A-B-C 10-bit packed planar */
    CY_PIXEL_FORMAT_Coord3D_ABC12p                      = 0x022400DE, /* 3D coordinate A-B-C 12-bit packed */
    CY_PIXEL_FORMAT_Coord3D_ABC12p_Planar               = 0x022400DF, /* 3D coordinate A-B-C 12-bit packed planar */
    CY_PIXEL_FORMAT_Coord3D_ABC16                       = 0x023000B9, /* 3D coordinate A-B-C 16-bit */
    CY_PIXEL_FORMAT_Coord3D_ABC16_Planar                = 0x023000BA, /* 3D coordinate A-B-C 16-bit planar */
    CY_PIXEL_FORMAT_Coord3D_ABC32f                      = 0x026000C0, /* 3D coordinate A-B-C 32-bit floating point */
    CY_PIXEL_FORMAT_Coord3D_ABC32f_Planar               = 0x026000C1, /* 3D coordinate A-B-C 32-bit floating point planar */
    CY_PIXEL_FORMAT_Coord3D_AC8                         = 0x021000B4, /* 3D coordinate A-C 8-bit */
    CY_PIXEL_FORMAT_Coord3D_AC8_Planar                  = 0x021000B5, /* 3D coordinate A-C 8-bit planar */
    CY_PIXEL_FORMAT_Coord3D_AC10p                       = 0x021400F0, /* 3D coordinate A-C 10-bit packed */
    CY_PIXEL_FORMAT_Coord3D_AC10p_Planar                = 0x021400F1, /* 3D coordinate A-C 10-bit packed planar */
    CY_PIXEL_FORMAT_Coord3D_AC12p                       = 0x021800F2, /* 3D coordinate A-C 12-bit packed */
    CY_PIXEL_FORMAT_Coord3D_AC12p_Planar                = 0x021800F3, /* 3D coordinate A-C 12-bit packed planar */
    CY_PIXEL_FORMAT_Coord3D_AC16                        = 0x022000BB, /* 3D coordinate A-C 16-bit */
    CY_PIXEL_FORMAT_Coord3D_AC16_Planar                 = 0x022000BC, /* 3D coordinate A-C 16-bit planar */
    CY_PIXEL_FORMAT_Coord3D_AC32f                       = 0x024000C2, /* 3D coordinate A-C 32-bit floating point */
    CY_PIXEL_FORMAT_Coord3D_AC32f_Planar                = 0x024000C3, /* 3D coordinate A-C 32-bit floating point planar */
    CY_PIXEL_FORMAT_Coord3D_A8                          = 0x010800AF, /* 3D coordinate A 8-bit */
    CY_PIXEL_FORMAT_Coord3D_A10p                        = 0x010A00D5, /* 3D coordinate A 10-bit packed */
    CY_PIXEL_FORMAT_Coord3D_A12p                        = 0x010C00D8, /* 3D coordinate A 12-bit packed */
    CY_PIXEL_FORMAT_Coord3D_A16                         = 0x011000B6, /* 3D coordinate A 16-bit */
    CY_PIXEL_FORMAT_Coord3D_A32f                        = 0x012000BD, /* 3D coordinate A 32-bit floating point */
    CY_PIXEL_FORMAT_Coord3D_B8                          = 0x010800B0, /* 3D coordinate B 8-bit */
    CY_PIXEL_FORMAT_Coord3D_B10p                        = 0x010A00D6, /* 3D coordinate B 10-bit packed */
    CY_PIXEL_FORMAT_Coord3D_B12p                        = 0x010C00D9, /* 3D coordinate B 12-bit packed */
    CY_PIXEL_FORMAT_Coord3D_B16                         = 0x011000B7, /* 3D coordinate B 16-bit */
    CY_PIXEL_FORMAT_Coord3D_B32f                        = 0x012000BE, /* 3D coordinate B 32-bit floating point */
    CY_PIXEL_FORMAT_Coord3D_C8                          = 0x010800B1, /* 3D coordinate C 8-bit */
    CY_PIXEL_FORMAT_Coord3D_C10p                        = 0x010A00D7, /* 3D coordinate C 10-bit packed */
    CY_PIXEL_FORMAT_Coord3D_C12p                        = 0x010C00DA, /* 3D coordinate C 12-bit packed */
    CY_PIXEL_FORMAT_Coord3D_C16                         = 0x011000B8, /* 3D coordinate C 16-bit */
    CY_PIXEL_FORMAT_Coord3D_C32f                        = 0x012000BF, /* 3D coordinate C 32-bit floating point */
    CY_PIXEL_FORMAT_Confidence1                         = 0x010800C4, /* Confidence 1-bit unpacked */
    CY_PIXEL_FORMAT_Confidence1p                        = 0x010100C5, /* Confidence 1-bit packed */
    CY_PIXEL_FORMAT_Confidence8                         = 0x010800C6, /* Confidence 8-bit */
    CY_PIXEL_FORMAT_Confidence16                        = 0x011000C7, /* Confidence 16-bit */
    CY_PIXEL_FORMAT_Confidence32f                       = 0x012000C8, /* Confidence 32-bit floating point */
    CY_PIXEL_FORMAT_BiColorBGRG8                        = 0x021000A6, /* Bi-color Blue/Green - Red/Green 8-bit */
    CY_PIXEL_FORMAT_BiColorBGRG10                       = 0x022000A9, /* Bi-color Blue/Green - Red/Green 10-bit unpacked */
    CY_PIXEL_FORMAT_BiColorBGRG10p                      = 0x021400AA, /* Bi-color Blue/Green - Red/Green 10-bit packed */
    CY_PIXEL_FORMAT_BiColorBGRG12                       = 0x022000AD, /* Bi-color Blue/Green - Red/Green 12-bit unpacked */
    CY_PIXEL_FORMAT_BiColorBGRG12p                      = 0x021800AE, /* Bi-color Blue/Green - Red/Green 12-bit packed */
    CY_PIXEL_FORMAT_BiColorRGBG8                        = 0x021000A5, /* Bi-color Red/Green - Blue/Green 8-bit */
    CY_PIXEL_FORMAT_BiColorRGBG10                       = 0x022000A7, /* Bi-color Red/Green - Blue/Green 10-bit unpacked */
    CY_PIXEL_FORMAT_BiColorRGBG10p                      = 0x021400A8, /* Bi-color Red/Green - Blue/Green 10-bit packed */
    CY_PIXEL_FORMAT_BiColorRGBG12                       = 0x022000AB, /* Bi-color Red/Green - Blue/Green 12-bit unpacked */
    CY_PIXEL_FORMAT_BiColorRGBG12p                      = 0x021800AC, /* Bi-color Red/Green - Blue/Green 12-bit packed */
    CY_PIXEL_FORMAT_Data8                               = 0x01080116, /* Data 8-bit */
    CY_PIXEL_FORMAT_Data8s                              = 0x01080117, /* Data 8-bit signed */
    CY_PIXEL_FORMAT_Data16                              = 0x01100118, /* Data 16-bit */
    CY_PIXEL_FORMAT_Data16s                             = 0x01100119, /* Data 16-bit signed */
    CY_PIXEL_FORMAT_Data32                              = 0x0120011A, /* Data 32-bit */
    CY_PIXEL_FORMAT_Data32f                             = 0x0120011C, /* Data 32-bit floating point */
    CY_PIXEL_FORMAT_Data32s                             = 0x0120011B, /* Data 32-bit signed */
    CY_PIXEL_FORMAT_Data64                              = 0x0140011D, /* Data 64-bit */
    CY_PIXEL_FORMAT_Data64f                             = 0x0140011F, /* Data 64-bit floating point */
    CY_PIXEL_FORMAT_Data64s                             = 0x0140011E, /* Data 64-bit signed */
    CY_PIXEL_FORMAT_SCF1WBWG8                           = 0x01080067, /* Sparse Color Filter #1 White-Blue-White-Green 8-bit */
    CY_PIXEL_FORMAT_SCF1WBWG10                          = 0x01100068, /* Sparse Color Filter #1 White-Blue-White-Green 10-bit unpacked */
    CY_PIXEL_FORMAT_SCF1WBWG10p                         = 0x010A0069, /* Sparse Color Filter #1 White-Blue-White-Green 10-bit packed */
    CY_PIXEL_FORMAT_SCF1WBWG12                          = 0x0110006A, /* Sparse Color Filter #1 White-Blue-White-Green 12-bit unpacked */
    CY_PIXEL_FORMAT_SCF1WBWG12p                         = 0x010C006B, /* Sparse Color Filter #1 White-Blue-White-Green 12-bit packed */
    CY_PIXEL_FORMAT_SCF1WBWG14                          = 0x0110006C, /* Sparse Color Filter #1 White-Blue-White-Green 14-bit unpacked */
    CY_PIXEL_FORMAT_SCF1WBWG16                          = 0x0110006D, /* Sparse Color Filter #1 White-Blue-White-Green 16-bit unpacked */
    CY_PIXEL_FORMAT_SCF1WGWB8                           = 0x0108006E, /* Sparse Color Filter #1 White-Green-White-Blue 8-bit */
    CY_PIXEL_FORMAT_SCF1WGWB10                          = 0x0110006F, /* Sparse Color Filter #1 White-Green-White-Blue 10-bit unpacked */
    CY_PIXEL_FORMAT_SCF1WGWB10p                         = 0x010A0070, /* Sparse Color Filter #1 White-Green-White-Blue 10-bit packed */
    CY_PIXEL_FORMAT_SCF1WGWB12                          = 0x01100071, /* Sparse Color Filter #1 White-Green-White-Blue 12-bit unpacked */
    CY_PIXEL_FORMAT_SCF1WGWB12p                         = 0x010C0072, /* Sparse Color Filter #1 White-Green-White-Blue 12-bit packed */
    CY_PIXEL_FORMAT_SCF1WGWB14                          = 0x01100073, /* Sparse Color Filter #1 White-Green-White-Blue 14-bit unpacked */
    CY_PIXEL_FORMAT_SCF1WGWB16                          = 0x01100074, /* Sparse Color Filter #1 White-Green-White-Blue 16-bit */
    CY_PIXEL_FORMAT_SCF1WGWR8                           = 0x01080075, /* Sparse Color Filter #1 White-Green-White-Red 8-bit */
    CY_PIXEL_FORMAT_SCF1WGWR10                          = 0x01100076, /* Sparse Color Filter #1 White-Green-White-Red 10-bit unpacked */
    CY_PIXEL_FORMAT_SCF1WGWR10p                         = 0x010A0077, /* Sparse Color Filter #1 White-Green-White-Red 10-bit packed */
    CY_PIXEL_FORMAT_SCF1WGWR12                          = 0x01100078, /* Sparse Color Filter #1 White-Green-White-Red 12-bit unpacked */
    CY_PIXEL_FORMAT_SCF1WGWR12p                         = 0x010C0079, /* Sparse Color Filter #1 White-Green-White-Red 12-bit packed */
    CY_PIXEL_FORMAT_SCF1WGWR14                          = 0x0110007A, /* Sparse Color Filter #1 White-Green-White-Red 14-bit unpacked */
    CY_PIXEL_FORMAT_SCF1WGWR16                          = 0x0110007B, /* Sparse Color Filter #1 White-Green-White-Red 16-bit */
    CY_PIXEL_FORMAT_SCF1WRWG8                           = 0x0108007C, /* Sparse Color Filter #1 White-Red-White-Green 8-bit */
    CY_PIXEL_FORMAT_SCF1WRWG10                          = 0x0110007D, /* Sparse Color Filter #1 White-Red-White-Green 10-bit unpacked */
    CY_PIXEL_FORMAT_SCF1WRWG10p                         = 0x010A007E, /* Sparse Color Filter #1 White-Red-White-Green 10-bit packed */
    CY_PIXEL_FORMAT_SCF1WRWG12                          = 0x0110007F, /* Sparse Color Filter #1 White-Red-White-Green 12-bit unpacked */
    CY_PIXEL_FORMAT_SCF1WRWG12p                         = 0x010C0080, /* Sparse Color Filter #1 White-Red-White-Green 12-bit packed */
    CY_PIXEL_FORMAT_SCF1WRWG14                          = 0x01100081, /* Sparse Color Filter #1 White-Red-White-Green 14-bit unpacked */
    CY_PIXEL_FORMAT_SCF1WRWG16                          = 0x01100082, /* Sparse Color Filter #1 White-Red-White-Green 16-bit */
    CY_PIXEL_FORMAT_YCbCr8                              = 0x0218005B, /* YCbCr 4:4:4 8-bit */
    CY_PIXEL_FORMAT_YCbCr8_CbYCr                        = 0x0218003A, /* YCbCr 4:4:4 8-bit */
    CY_PIXEL_FORMAT_YCbCr10_CbYCr                       = 0x02300083, /* YCbCr 4:4:4 10-bit unpacked */
    CY_PIXEL_FORMAT_YCbCr10p_CbYCr                      = 0x021E0084, /* YCbCr 4:4:4 10-bit packed */
    CY_PIXEL_FORMAT_YCbCr12_CbYCr                       = 0x02300085, /* YCbCr 4:4:4 12-bit unpacked */
    CY_PIXEL_FORMAT_YCbCr12p_CbYCr                      = 0x02240086, /* YCbCr 4:4:4 12-bit packed */
    CY_PIXEL_FORMAT_YCbCr411_8                          = 0x020C005A, /* YCbCr 4:1:1 8-bit */
    CY_PIXEL_FORMAT_YCbCr411_8_CbYYCrYY                 = 0x020C003C, /* YCbCr 4:1:1 8-bit */
    CY_PIXEL_FORMAT_YCbCr420_8_YY_CbCr_Semiplanar       = 0x020C0112, /* YCbCr 4:2:0 8-bit YY/CbCr Semiplanar */
    CY_PIXEL_FORMAT_YCbCr420_8_YY_CrCb_Semiplanar       = 0x020C0114, /* YCbCr 4:2:0 8-bit YY/CrCb Semiplanar */
    CY_PIXEL_FORMAT_YCbCr422_8                          = 0x0210003B, /* YCbCr 4:2:2 8-bit */
    CY_PIXEL_FORMAT_YCbCr422_8_CbYCrY                   = 0x02100043, /* YCbCr 4:2:2 8-bit */
    CY_PIXEL_FORMAT_YCbCr422_8_YY_CbCr_Semiplanar       = 0x02100113, /* YCbCr 4:2:2 8-bit YY/CbCr Semiplanar */
    CY_PIXEL_FORMAT_YCbCr422_8_YY_CrCb_Semiplanar       = 0x02100115, /* YCbCr 4:2:2 8-bit YY/CrCb Semiplanar */
    CY_PIXEL_FORMAT_YCbCr422_10                         = 0x02200065, /* YCbCr 4:2:2 10-bit unpacked */
    CY_PIXEL_FORMAT_YCbCr422_10_CbYCrY                  = 0x02200099, /* YCbCr 4:2:2 10-bit unpacked */
    CY_PIXEL_FORMAT_YCbCr422_10p                        = 0x02140087, /* YCbCr 4:2:2 10-bit packed */
    CY_PIXEL_FORMAT_YCbCr422_10p_CbYCrY                 = 0x0214009A, /* YCbCr 4:2:2 10-bit packed */
    CY_PIXEL_FORMAT_YCbCr422_12                         = 0x02200066, /* YCbCr 4:2:2 12-bit unpacked */
    CY_PIXEL_FORMAT_YCbCr422_12_CbYCrY                  = 0x0220009B, /* YCbCr 4:2:2 12-bit unpacked */
    CY_PIXEL_FORMAT_YCbCr422_12p                        = 0x02180088, /* YCbCr 4:2:2 12-bit packed */
    CY_PIXEL_FORMAT_YCbCr422_12p_CbYCrY                 = 0x0218009C, /* YCbCr 4:2:2 12-bit packed */
    CY_PIXEL_FORMAT_YCbCr601_8_CbYCr                    = 0x0218003D, /* YCbCr 4:4:4 8-bit BT.601 */
    CY_PIXEL_FORMAT_YCbCr601_10_CbYCr                   = 0x02300089, /* YCbCr 4:4:4 10-bit unpacked BT.601 */
    CY_PIXEL_FORMAT_YCbCr601_10p_CbYCr                  = 0x021E008A, /* YCbCr 4:4:4 10-bit packed BT.601 */
    CY_PIXEL_FORMAT_YCbCr601_12_CbYCr                   = 0x0230008B, /* YCbCr 4:4:4 12-bit unpacked BT.601 */
    CY_PIXEL_FORMAT_YCbCr601_12p_CbYCr                  = 0x0224008C, /* YCbCr 4:4:4 12-bit packed BT.601 */
    CY_PIXEL_FORMAT_YCbCr601_411_8_CbYYCrYY             = 0x020C003F, /* YCbCr 4:1:1 8-bit BT.601 */
    CY_PIXEL_FORMAT_YCbCr601_422_8                      = 0x0210003E, /* YCbCr 4:2:2 8-bit BT.601 */
    CY_PIXEL_FORMAT_YCbCr601_422_8_CbYCrY               = 0x02100044, /* YCbCr 4:2:2 8-bit BT.601 */
    CY_PIXEL_FORMAT_YCbCr601_422_10                     = 0x0220008D, /* YCbCr 4:2:2 10-bit unpacked BT.601 */
    CY_PIXEL_FORMAT_YCbCr601_422_10_CbYCrY              = 0x0220009D, /* YCbCr 4:2:2 10-bit unpacked BT.601 */
    CY_PIXEL_FORMAT_YCbCr601_422_10p                    = 0x0214008E, /* YCbCr 4:2:2 10-bit packed BT.601 */
    CY_PIXEL_FORMAT_YCbCr601_422_10p_CbYCrY             = 0x0214009E, /* YCbCr 4:2:2 10-bit packed BT.601 */
    CY_PIXEL_FORMAT_YCbCr601_422_12                     = 0x0220008F, /* YCbCr 4:2:2 12-bit unpacked BT.601 */
    CY_PIXEL_FORMAT_YCbCr601_422_12_CbYCrY              = 0x0220009F, /* YCbCr 4:2:2 12-bit unpacked BT.601 */
    CY_PIXEL_FORMAT_YCbCr601_422_12p                    = 0x02180090, /* YCbCr 4:2:2 12-bit packed BT.601 */
    CY_PIXEL_FORMAT_YCbCr601_422_12p_CbYCrY             = 0x021800A0, /* YCbCr 4:2:2 12-bit packed BT.601 */
    CY_PIXEL_FORMAT_YCbCr709_8_CbYCr                    = 0x02180040, /* YCbCr 4:4:4 8-bit BT.709 */
    CY_PIXEL_FORMAT_YCbCr709_10_CbYCr                   = 0x02300091, /* YCbCr 4:4:4 10-bit unpacked BT.709 */
    CY_PIXEL_FORMAT_YCbCr709_10p_CbYCr                  = 0x021E0092, /* YCbCr 4:4:4 10-bit packed BT.709 */
    CY_PIXEL_FORMAT_YCbCr709_12_CbYCr                   = 0x02300093, /* YCbCr 4:4:4 12-bit unpacked BT.709 */
    CY_PIXEL_FORMAT_YCbCr709_12p_CbYCr                  = 0x02240094, /* YCbCr 4:4:4 12-bit packed BT.709 */
    CY_PIXEL_FORMAT_YCbCr709_411_8_CbYYCrYY             = 0x020C0042, /* YCbCr 4:1:1 8-bit BT.709 */
    CY_PIXEL_FORMAT_YCbCr709_422_8                      = 0x02100041, /* YCbCr 4:2:2 8-bit BT.709 */
    CY_PIXEL_FORMAT_YCbCr709_422_8_CbYCrY               = 0x02100045, /* YCbCr 4:2:2 8-bit BT.709 */
    CY_PIXEL_FORMAT_YCbCr709_422_10                     = 0x02200095, /* YCbCr 4:2:2 10-bit unpacked BT.709 */
    CY_PIXEL_FORMAT_YCbCr709_422_10_CbYCrY              = 0x022000A1, /* YCbCr 4:2:2 10-bit unpacked BT.709 */
    CY_PIXEL_FORMAT_YCbCr709_422_10p                    = 0x02140096, /* YCbCr 4:2:2 10-bit packed BT.709 */
    CY_PIXEL_FORMAT_YCbCr709_422_10p_CbYCrY             = 0x021400A2, /* YCbCr 4:2:2 10-bit packed BT.709 */
    CY_PIXEL_FORMAT_YCbCr709_422_12                     = 0x02200097, /* YCbCr 4:2:2 12-bit unpacked BT.709 */
    CY_PIXEL_FORMAT_YCbCr709_422_12_CbYCrY              = 0x022000A3, /* YCbCr 4:2:2 12-bit unpacked BT.709 */
    CY_PIXEL_FORMAT_YCbCr709_422_12p                    = 0x02180098, /* YCbCr 4:2:2 12-bit packed BT.709 */
    CY_PIXEL_FORMAT_YCbCr709_422_12p_CbYCrY             = 0x021800A4, /* YCbCr 4:2:2 12-bit packed BT.709 */
    CY_PIXEL_FORMAT_YCbCr2020_8_CbYCr                   = 0x021800F4, /* YCbCr 4:4:4 8-bit BT.2020 */
    CY_PIXEL_FORMAT_YCbCr2020_10_CbYCr                  = 0x023000F5, /* YCbCr 4:4:4 10-bit unpacked BT.2020 */
    CY_PIXEL_FORMAT_YCbCr2020_10p_CbYCr                 = 0x021E00F6, /* YCbCr 4:4:4 10-bit packed BT.2020 */
    CY_PIXEL_FORMAT_YCbCr2020_12_CbYCr                  = 0x023000F7, /* YCbCr 4:4:4 12-bit unpacked BT.2020 */
    CY_PIXEL_FORMAT_YCbCr2020_12p_CbYCr                 = 0x022400F8, /* YCbCr 4:4:4 12-bit packed BT.2020 */
    CY_PIXEL_FORMAT_YCbCr2020_411_8_CbYYCrYY            = 0x020C00F9, /* YCbCr 4:1:1 8-bit BT.2020 */
    CY_PIXEL_FORMAT_YCbCr2020_422_8                     = 0x021000FA, /* YCbCr 4:2:2 8-bit BT.2020 */
    CY_PIXEL_FORMAT_YCbCr2020_422_8_CbYCrY              = 0x021000FB, /* YCbCr 4:2:2 8-bit BT.2020 */
    CY_PIXEL_FORMAT_YCbCr2020_422_10                    = 0x022000FC, /* YCbCr 4:2:2 10-bit unpacked BT.2020 */
    CY_PIXEL_FORMAT_YCbCr2020_422_10_CbYCrY             = 0x022000FD, /* YCbCr 4:2:2 10-bit unpacked BT.2020 */
    CY_PIXEL_FORMAT_YCbCr2020_422_10p                   = 0x021400FE, /* YCbCr 4:2:2 10-bit packed BT.2020 */
    CY_PIXEL_FORMAT_YCbCr2020_422_10p_CbYCrY            = 0x021400FF, /* YCbCr 4:2:2 10-bit packed BT.2020 */
    CY_PIXEL_FORMAT_YCbCr2020_422_12                    = 0x02200100, /* YCbCr 4:2:2 12-bit unpacked BT.2020 */
    CY_PIXEL_FORMAT_YCbCr2020_422_12_CbYCrY             = 0x02200101, /* YCbCr 4:2:2 12-bit unpacked BT.2020 */
    CY_PIXEL_FORMAT_YCbCr2020_422_12p                   = 0x02180102, /* YCbCr 4:2:2 12-bit packed BT.2020 */
    CY_PIXEL_FORMAT_YCbCr2020_422_12p_CbYCrY            = 0x02180103, /* YCbCr 4:2:2 12-bit packed BT.2020 */
    CY_PIXEL_FORMAT_YUV8_UYV                            = 0x02180020, /* YUV 4:4:4 8-bit */
    CY_PIXEL_FORMAT_YUV411_8_UYYVYY                     = 0x020C001E, /* YUV 4:1:1 8-bit */
    CY_PIXEL_FORMAT_YUV422_8                            = 0x02100032, /* YUV 4:2:2 8-bit */
    CY_PIXEL_FORMAT_YUV422_8_UYVY                       = 0x0210001F, /* YUV 4:2:2 8-bit */

    CY_PIXEL_FORMAT_GVSP_Mono10Packed                   = 0x010C0004, /* GigE Vision specific format, Monochrome 10-bit packed */
    CY_PIXEL_FORMAT_GVSP_Mono12Packed                   = 0x010C0006, /* GigE Vision specific format, Monochrome 12-bit packed */
    CY_PIXEL_FORMAT_GVSP_BayerBG10Packed                = 0x010C0029, /* GigE Vision specific format, Bayer Blue-Green 10-bit packed */
    CY_PIXEL_FORMAT_GVSP_BayerBG12Packed                = 0x010C002D, /* GigE Vision specific format, Bayer Blue-Green 12-bit packed */
    CY_PIXEL_FORMAT_GVSP_BayerGB10Packed                = 0x010C0028, /* GigE Vision specific format, Bayer Green-Blue 10-bit packed */
    CY_PIXEL_FORMAT_GVSP_BayerGB12Packed                = 0x010C002C, /* GigE Vision specific format, Bayer Green-Blue 12-bit packed */
    CY_PIXEL_FORMAT_GVSP_BayerGR10Packed                = 0x010C0026, /* GigE Vision specific format, Bayer Green-Red 10-bit packed */
    CY_PIXEL_FORMAT_GVSP_BayerGR12Packed                = 0x010C002A, /* GigE Vision specific format, Bayer Green-Red 12-bit packed */
    CY_PIXEL_FORMAT_GVSP_BayerRG10Packed                = 0x010C0027, /* GigE Vision specific format, Bayer Red-Green 10-bit packed */
    CY_PIXEL_FORMAT_GVSP_BayerRG12Packed                = 0x010C002B, /* GigE Vision specific format, Bayer Red-Green 12-bit packed */
    CY_PIXEL_FORMAT_GVSP_RGB10V1Packed                  = 0x0220001C, /* GigE Vision specific format, Red-Green-Blue 10-bit packed - variant 1 */
    CY_PIXEL_FORMAT_GVSP_RGB12V1Packed                  = 0x02240034, /* GigE Vision specific format, Red-Green-Blue 12-bit packed - variant 1 */

    CY_PIXEL_FORMAT_Invalid                             = 0
}CY_PIXEL_FORMAT;

/* 32-bit value layout */
/* |31            24|23            16|15            08|07            00| */
/* | C| Comp. Layout| Effective Size |            Pixel ID             | */

/* Custom flag */
#define CY_PIXEL_FORMAT_CUSTOM                              0x80000000
/* Component layout */
#define CY_PIXEL_FORMAT_SINGLE_COMPONENT                    0x01000000
#define CY_PIXEL_FORMAT_MULTIPLE_COMPONENT                  0x02000000
#define CY_PIXEL_FORMAT_COMPONENT_MASK                      0x7F000000
/* Effective size */
#define CY_PIXEL_FORMAT_OCCUPY1BIT                          0x00010000
#define CY_PIXEL_FORMAT_OCCUPY2BIT                          0x00020000
#define CY_PIXEL_FORMAT_OCCUPY4BIT                          0x00040000
#define CY_PIXEL_FORMAT_OCCUPY8BIT                          0x00080000
#define CY_PIXEL_FORMAT_OCCUPY10BIT                         0x000A0000
#define CY_PIXEL_FORMAT_OCCUPY12BIT                         0x000C0000
#define CY_PIXEL_FORMAT_OCCUPY16BIT                         0x00100000
#define CY_PIXEL_FORMAT_OCCUPY24BIT                         0x00180000
#define CY_PIXEL_FORMAT_OCCUPY30BIT                         0x001E0000
#define CY_PIXEL_FORMAT_OCCUPY32BIT                         0x00200000
#define CY_PIXEL_FORMAT_OCCUPY36BIT                         0x00240000
#define CY_PIXEL_FORMAT_OCCUPY40BIT                         0x00280000
#define CY_PIXEL_FORMAT_OCCUPY48BIT                         0x00300000
#define CY_PIXEL_FORMAT_OCCUPY64BIT                         0x00400000
#define CY_PIXEL_FORMAT_PIXEL_SIZE_MASK                     0x00FF0000
#define CY_PIXEL_FORMAT_PIXEL_SIZE_SHIFT                    16
/* Pixel ID */
#define CY_PIXEL_FORMAT_PIXEL_ID_MASK                       0x0000FFFF
/* Pixel format value dissection helpers */
#define CY_PIXEL_FORMAT_BPP(X)                              ((X & CY_PIXEL_FORMAT_PIXEL_SIZE_MASK) >> CY_PIXEL_FORMAT_PIXEL_SIZE_SHIFT)
#define CY_PIXEL_FORMAT_IS_PIXEL_SINGLE_COMPONENT(X)        ((X & CY_PIXEL_FORMAT_COMPONENT_MASK) == CY_PIXEL_FORMAT_SINGLE_COMPONENT)
#define CY_PIXEL_FORMAT_IS_PIXEL_MULTIPLE_COMPONENT(X)      ((X & CY_PIXEL_FORMAT_COMPONENT_MASK) == CY_PIXEL_FORMAT_MULTIPLE_COMPONENT)
#define CY_PIXEL_FORMAT_IS_PIXEL_CUSTOM(X)                  ((X & CY_PIXEL_FORMAT_CUSTOM) == CY_PIXEL_FORMAT_CUSTOM)
#define CY_PIXEL_FORMAT_PIXEL_ID(X)                         (X & CY_PIXEL_FORMAT_PIXEL_ID_MASK)

/* Additional helpers */
#ifdef CY_PF_INCLUDE_HELPERS
#ifdef _MSC_VER
#  define CY_PF_INLINE __inline
#else
#  define CY_PF_INLINE inline
#endif
static CY_PF_INLINE const char* CY_GetPixelFormatName(CY_PIXEL_FORMAT format)
{
  switch (format)
  {
    case CY_PIXEL_FORMAT_Mono1p:                                  return "Mono1p";
    case CY_PIXEL_FORMAT_Mono2p:                                  return "Mono2p";
    case CY_PIXEL_FORMAT_Mono4p:                                  return "Mono4p";
    case CY_PIXEL_FORMAT_Mono8:                                   return "Mono8";
    case CY_PIXEL_FORMAT_Mono8s:                                  return "Mono8s";
    case CY_PIXEL_FORMAT_Mono10:                                  return "Mono10";
    case CY_PIXEL_FORMAT_Mono10p:                                 return "Mono10p";
    case CY_PIXEL_FORMAT_Mono12:                                  return "Mono12";
    case CY_PIXEL_FORMAT_Mono12p:                                 return "Mono12p";
    case CY_PIXEL_FORMAT_Mono14:                                  return "Mono14";
    case CY_PIXEL_FORMAT_Mono14p:                                 return "Mono14p";
    case CY_PIXEL_FORMAT_Mono16:                                  return "Mono16";
    case CY_PIXEL_FORMAT_Mono32:                                  return "Mono32";
    case CY_PIXEL_FORMAT_BayerBG4p:                               return "BayerBG4p";
    case CY_PIXEL_FORMAT_BayerBG8:                                return "BayerBG8";
    case CY_PIXEL_FORMAT_BayerBG10:                               return "BayerBG10";
    case CY_PIXEL_FORMAT_BayerBG10p:                              return "BayerBG10p";
    case CY_PIXEL_FORMAT_BayerBG12:                               return "BayerBG12";
    case CY_PIXEL_FORMAT_BayerBG12p:                              return "BayerBG12p";
    case CY_PIXEL_FORMAT_BayerBG14:                               return "BayerBG14";
    case CY_PIXEL_FORMAT_BayerBG14p:                              return "BayerBG14p";
    case CY_PIXEL_FORMAT_BayerBG16:                               return "BayerBG16";
    case CY_PIXEL_FORMAT_BayerGB4p:                               return "BayerGB4p";
    case CY_PIXEL_FORMAT_BayerGB8:                                return "BayerGB8";
    case CY_PIXEL_FORMAT_BayerGB10:                               return "BayerGB10";
    case CY_PIXEL_FORMAT_BayerGB10p:                              return "BayerGB10p";
    case CY_PIXEL_FORMAT_BayerGB12:                               return "BayerGB12";
    case CY_PIXEL_FORMAT_BayerGB12p:                              return "BayerGB12p";
    case CY_PIXEL_FORMAT_BayerGB14:                               return "BayerGB14";
    case CY_PIXEL_FORMAT_BayerGB14p:                              return "BayerGB14p";
    case CY_PIXEL_FORMAT_BayerGB16:                               return "BayerGB16";
    case CY_PIXEL_FORMAT_BayerGR4p:                               return "BayerGR4p";
    case CY_PIXEL_FORMAT_BayerGR8:                                return "BayerGR8";
    case CY_PIXEL_FORMAT_BayerGR10:                               return "BayerGR10";
    case CY_PIXEL_FORMAT_BayerGR10p:                              return "BayerGR10p";
    case CY_PIXEL_FORMAT_BayerGR12:                               return "BayerGR12";
    case CY_PIXEL_FORMAT_BayerGR12p:                              return "BayerGR12p";
    case CY_PIXEL_FORMAT_BayerGR14:                               return "BayerGR14";
    case CY_PIXEL_FORMAT_BayerGR14p:                              return "BayerGR14p";
    case CY_PIXEL_FORMAT_BayerGR16:                               return "BayerGR16";
    case CY_PIXEL_FORMAT_BayerRG4p:                               return "BayerRG4p";
    case CY_PIXEL_FORMAT_BayerRG8:                                return "BayerRG8";
    case CY_PIXEL_FORMAT_BayerRG10:                               return "BayerRG10";
    case CY_PIXEL_FORMAT_BayerRG10p:                              return "BayerRG10p";
    case CY_PIXEL_FORMAT_BayerRG12:                               return "BayerRG12";
    case CY_PIXEL_FORMAT_BayerRG12p:                              return "BayerRG12p";
    case CY_PIXEL_FORMAT_BayerRG14:                               return "BayerRG14";
    case CY_PIXEL_FORMAT_BayerRG14p:                              return "BayerRG14p";
    case CY_PIXEL_FORMAT_BayerRG16:                               return "BayerRG16";
    case CY_PIXEL_FORMAT_RGBa8:                                   return "RGBa8";
    case CY_PIXEL_FORMAT_RGBa10:                                  return "RGBa10";
    case CY_PIXEL_FORMAT_RGBa10p:                                 return "RGBa10p";
    case CY_PIXEL_FORMAT_RGBa12:                                  return "RGBa12";
    case CY_PIXEL_FORMAT_RGBa12p:                                 return "RGBa12p";
    case CY_PIXEL_FORMAT_RGBa14:                                  return "RGBa14";
    case CY_PIXEL_FORMAT_RGBa16:                                  return "RGBa16";
    case CY_PIXEL_FORMAT_RGB8:                                    return "RGB8";
    case CY_PIXEL_FORMAT_RGB8_Planar:                             return "RGB8_Planar";
    case CY_PIXEL_FORMAT_RGB10:                                   return "RGB10";
    case CY_PIXEL_FORMAT_RGB10_Planar:                            return "RGB10_Planar";
    case CY_PIXEL_FORMAT_RGB10p:                                  return "RGB10p";
    case CY_PIXEL_FORMAT_RGB10p32:                                return "RGB10p32";
    case CY_PIXEL_FORMAT_RGB12:                                   return "RGB12";
    case CY_PIXEL_FORMAT_RGB12_Planar:                            return "RGB12_Planar";
    case CY_PIXEL_FORMAT_RGB12p:                                  return "RGB12p";
    case CY_PIXEL_FORMAT_RGB14:                                   return "RGB14";
    case CY_PIXEL_FORMAT_RGB16:                                   return "RGB16";
    case CY_PIXEL_FORMAT_RGB16_Planar:                            return "RGB16_Planar";
    case CY_PIXEL_FORMAT_RGB565p:                                 return "RGB565p";
    case CY_PIXEL_FORMAT_BGRa8:                                   return "BGRa8";
    case CY_PIXEL_FORMAT_BGRa10:                                  return "BGRa10";
    case CY_PIXEL_FORMAT_BGRa10p:                                 return "BGRa10p";
    case CY_PIXEL_FORMAT_BGRa12:                                  return "BGRa12";
    case CY_PIXEL_FORMAT_BGRa12p:                                 return "BGRa12p";
    case CY_PIXEL_FORMAT_BGRa14:                                  return "BGRa14";
    case CY_PIXEL_FORMAT_BGRa16:                                  return "BGRa16";
    case CY_PIXEL_FORMAT_BGR8:                                    return "BGR8";
    case CY_PIXEL_FORMAT_BGR10:                                   return "BGR10";
    case CY_PIXEL_FORMAT_BGR10p:                                  return "BGR10p";
    case CY_PIXEL_FORMAT_BGR12:                                   return "BGR12";
    case CY_PIXEL_FORMAT_BGR12p:                                  return "BGR12p";
    case CY_PIXEL_FORMAT_BGR14:                                   return "BGR14";
    case CY_PIXEL_FORMAT_BGR16:                                   return "BGR16";
    case CY_PIXEL_FORMAT_BGR565p:                                 return "BGR565p";
    case CY_PIXEL_FORMAT_R8:                                      return "R8";
    case CY_PIXEL_FORMAT_R10:                                     return "R10";
    case CY_PIXEL_FORMAT_R12:                                     return "R12";
    case CY_PIXEL_FORMAT_R16:                                     return "R16";
    case CY_PIXEL_FORMAT_G8:                                      return "G8";
    case CY_PIXEL_FORMAT_G10:                                     return "G10";
    case CY_PIXEL_FORMAT_G12:                                     return "G12";
    case CY_PIXEL_FORMAT_G16:                                     return "G16";
    case CY_PIXEL_FORMAT_B8:                                      return "B8";
    case CY_PIXEL_FORMAT_B10:                                     return "B10";
    case CY_PIXEL_FORMAT_B12:                                     return "B12";
    case CY_PIXEL_FORMAT_B16:                                     return "B16";
    case CY_PIXEL_FORMAT_Coord3D_ABC8:                            return "Coord3D_ABC8";
    case CY_PIXEL_FORMAT_Coord3D_ABC8_Planar:                     return "Coord3D_ABC8_Planar";
    case CY_PIXEL_FORMAT_Coord3D_ABC10p:                          return "Coord3D_ABC10p";
    case CY_PIXEL_FORMAT_Coord3D_ABC10p_Planar:                   return "Coord3D_ABC10p_Planar";
    case CY_PIXEL_FORMAT_Coord3D_ABC12p:                          return "Coord3D_ABC12p";
    case CY_PIXEL_FORMAT_Coord3D_ABC12p_Planar:                   return "Coord3D_ABC12p_Planar";
    case CY_PIXEL_FORMAT_Coord3D_ABC16:                           return "Coord3D_ABC16";
    case CY_PIXEL_FORMAT_Coord3D_ABC16_Planar:                    return "Coord3D_ABC16_Planar";
    case CY_PIXEL_FORMAT_Coord3D_ABC32f:                          return "Coord3D_ABC32f";
    case CY_PIXEL_FORMAT_Coord3D_ABC32f_Planar:                   return "Coord3D_ABC32f_Planar";
    case CY_PIXEL_FORMAT_Coord3D_AC8:                             return "Coord3D_AC8";
    case CY_PIXEL_FORMAT_Coord3D_AC8_Planar:                      return "Coord3D_AC8_Planar";
    case CY_PIXEL_FORMAT_Coord3D_AC10p:                           return "Coord3D_AC10p";
    case CY_PIXEL_FORMAT_Coord3D_AC10p_Planar:                    return "Coord3D_AC10p_Planar";
    case CY_PIXEL_FORMAT_Coord3D_AC12p:                           return "Coord3D_AC12p";
    case CY_PIXEL_FORMAT_Coord3D_AC12p_Planar:                    return "Coord3D_AC12p_Planar";
    case CY_PIXEL_FORMAT_Coord3D_AC16:                            return "Coord3D_AC16";
    case CY_PIXEL_FORMAT_Coord3D_AC16_Planar:                     return "Coord3D_AC16_Planar";
    case CY_PIXEL_FORMAT_Coord3D_AC32f:                           return "Coord3D_AC32f";
    case CY_PIXEL_FORMAT_Coord3D_AC32f_Planar:                    return "Coord3D_AC32f_Planar";
    case CY_PIXEL_FORMAT_Coord3D_A8:                              return "Coord3D_A8";
    case CY_PIXEL_FORMAT_Coord3D_A10p:                            return "Coord3D_A10p";
    case CY_PIXEL_FORMAT_Coord3D_A12p:                            return "Coord3D_A12p";
    case CY_PIXEL_FORMAT_Coord3D_A16:                             return "Coord3D_A16";
    case CY_PIXEL_FORMAT_Coord3D_A32f:                            return "Coord3D_A32f";
    case CY_PIXEL_FORMAT_Coord3D_B8:                              return "Coord3D_B8";
    case CY_PIXEL_FORMAT_Coord3D_B10p:                            return "Coord3D_B10p";
    case CY_PIXEL_FORMAT_Coord3D_B12p:                            return "Coord3D_B12p";
    case CY_PIXEL_FORMAT_Coord3D_B16:                             return "Coord3D_B16";
    case CY_PIXEL_FORMAT_Coord3D_B32f:                            return "Coord3D_B32f";
    case CY_PIXEL_FORMAT_Coord3D_C8:                              return "Coord3D_C8";
    case CY_PIXEL_FORMAT_Coord3D_C10p:                            return "Coord3D_C10p";
    case CY_PIXEL_FORMAT_Coord3D_C12p:                            return "Coord3D_C12p";
    case CY_PIXEL_FORMAT_Coord3D_C16:                             return "Coord3D_C16";
    case CY_PIXEL_FORMAT_Coord3D_C32f:                            return "Coord3D_C32f";
    case CY_PIXEL_FORMAT_Confidence1:                             return "Confidence1";
    case CY_PIXEL_FORMAT_Confidence1p:                            return "Confidence1p";
    case CY_PIXEL_FORMAT_Confidence8:                             return "Confidence8";
    case CY_PIXEL_FORMAT_Confidence16:                            return "Confidence16";
    case CY_PIXEL_FORMAT_Confidence32f:                           return "Confidence32f";
    case CY_PIXEL_FORMAT_BiColorBGRG8:                            return "BiColorBGRG8";
    case CY_PIXEL_FORMAT_BiColorBGRG10:                           return "BiColorBGRG10";
    case CY_PIXEL_FORMAT_BiColorBGRG10p:                          return "BiColorBGRG10p";
    case CY_PIXEL_FORMAT_BiColorBGRG12:                           return "BiColorBGRG12";
    case CY_PIXEL_FORMAT_BiColorBGRG12p:                          return "BiColorBGRG12p";
    case CY_PIXEL_FORMAT_BiColorRGBG8:                            return "BiColorRGBG8";
    case CY_PIXEL_FORMAT_BiColorRGBG10:                           return "BiColorRGBG10";
    case CY_PIXEL_FORMAT_BiColorRGBG10p:                          return "BiColorRGBG10p";
    case CY_PIXEL_FORMAT_BiColorRGBG12:                           return "BiColorRGBG12";
    case CY_PIXEL_FORMAT_BiColorRGBG12p:                          return "BiColorRGBG12p";
    case CY_PIXEL_FORMAT_Data8:                                   return "Data8";
    case CY_PIXEL_FORMAT_Data8s:                                  return "Data8s";
    case CY_PIXEL_FORMAT_Data16:                                  return "Data16";
    case CY_PIXEL_FORMAT_Data16s:                                 return "Data16s";
    case CY_PIXEL_FORMAT_Data32:                                  return "Data32";
    case CY_PIXEL_FORMAT_Data32f:                                 return "Data32f";
    case CY_PIXEL_FORMAT_Data32s:                                 return "Data32s";
    case CY_PIXEL_FORMAT_Data64:                                  return "Data64";
    case CY_PIXEL_FORMAT_Data64f:                                 return "Data64f";
    case CY_PIXEL_FORMAT_Data64s:                                 return "Data64s";
    case CY_PIXEL_FORMAT_SCF1WBWG8:                               return "SCF1WBWG8";
    case CY_PIXEL_FORMAT_SCF1WBWG10:                              return "SCF1WBWG10";
    case CY_PIXEL_FORMAT_SCF1WBWG10p:                             return "SCF1WBWG10p";
    case CY_PIXEL_FORMAT_SCF1WBWG12:                              return "SCF1WBWG12";
    case CY_PIXEL_FORMAT_SCF1WBWG12p:                             return "SCF1WBWG12p";
    case CY_PIXEL_FORMAT_SCF1WBWG14:                              return "SCF1WBWG14";
    case CY_PIXEL_FORMAT_SCF1WBWG16:                              return "SCF1WBWG16";
    case CY_PIXEL_FORMAT_SCF1WGWB8:                               return "SCF1WGWB8";
    case CY_PIXEL_FORMAT_SCF1WGWB10:                              return "SCF1WGWB10";
    case CY_PIXEL_FORMAT_SCF1WGWB10p:                             return "SCF1WGWB10p";
    case CY_PIXEL_FORMAT_SCF1WGWB12:                              return "SCF1WGWB12";
    case CY_PIXEL_FORMAT_SCF1WGWB12p:                             return "SCF1WGWB12p";
    case CY_PIXEL_FORMAT_SCF1WGWB14:                              return "SCF1WGWB14";
    case CY_PIXEL_FORMAT_SCF1WGWB16:                              return "SCF1WGWB16";
    case CY_PIXEL_FORMAT_SCF1WGWR8:                               return "SCF1WGWR8";
    case CY_PIXEL_FORMAT_SCF1WGWR10:                              return "SCF1WGWR10";
    case CY_PIXEL_FORMAT_SCF1WGWR10p:                             return "SCF1WGWR10p";
    case CY_PIXEL_FORMAT_SCF1WGWR12:                              return "SCF1WGWR12";
    case CY_PIXEL_FORMAT_SCF1WGWR12p:                             return "SCF1WGWR12p";
    case CY_PIXEL_FORMAT_SCF1WGWR14:                              return "SCF1WGWR14";
    case CY_PIXEL_FORMAT_SCF1WGWR16:                              return "SCF1WGWR16";
    case CY_PIXEL_FORMAT_SCF1WRWG8:                               return "SCF1WRWG8";
    case CY_PIXEL_FORMAT_SCF1WRWG10:                              return "SCF1WRWG10";
    case CY_PIXEL_FORMAT_SCF1WRWG10p:                             return "SCF1WRWG10p";
    case CY_PIXEL_FORMAT_SCF1WRWG12:                              return "SCF1WRWG12";
    case CY_PIXEL_FORMAT_SCF1WRWG12p:                             return "SCF1WRWG12p";
    case CY_PIXEL_FORMAT_SCF1WRWG14:                              return "SCF1WRWG14";
    case CY_PIXEL_FORMAT_SCF1WRWG16:                              return "SCF1WRWG16";
    case CY_PIXEL_FORMAT_YCbCr8:                                  return "YCbCr8";
    case CY_PIXEL_FORMAT_YCbCr8_CbYCr:                            return "YCbCr8_CbYCr";
    case CY_PIXEL_FORMAT_YCbCr10_CbYCr:                           return "YCbCr10_CbYCr";
    case CY_PIXEL_FORMAT_YCbCr10p_CbYCr:                          return "YCbCr10p_CbYCr";
    case CY_PIXEL_FORMAT_YCbCr12_CbYCr:                           return "YCbCr12_CbYCr";
    case CY_PIXEL_FORMAT_YCbCr12p_CbYCr:                          return "YCbCr12p_CbYCr";
    case CY_PIXEL_FORMAT_YCbCr411_8:                              return "YCbCr411_8";
    case CY_PIXEL_FORMAT_YCbCr411_8_CbYYCrYY:                     return "YCbCr411_8_CbYYCrYY";
    case CY_PIXEL_FORMAT_YCbCr420_8_YY_CbCr_Semiplanar:           return "YCbCr420_8_YY_CbCr_Semiplanar";
    case CY_PIXEL_FORMAT_YCbCr420_8_YY_CrCb_Semiplanar:           return "YCbCr420_8_YY_CrCb_Semiplanar";
    case CY_PIXEL_FORMAT_YCbCr422_8:                              return "YCbCr422_8";
    case CY_PIXEL_FORMAT_YCbCr422_8_CbYCrY:                       return "YCbCr422_8_CbYCrY";
    case CY_PIXEL_FORMAT_YCbCr422_8_YY_CbCr_Semiplanar:           return "YCbCr422_8_YY_CbCr_Semiplanar";
    case CY_PIXEL_FORMAT_YCbCr422_8_YY_CrCb_Semiplanar:           return "YCbCr422_8_YY_CrCb_Semiplanar";
    case CY_PIXEL_FORMAT_YCbCr422_10:                             return "YCbCr422_10";
    case CY_PIXEL_FORMAT_YCbCr422_10_CbYCrY:                      return "YCbCr422_10_CbYCrY";
    case CY_PIXEL_FORMAT_YCbCr422_10p:                            return "YCbCr422_10p";
    case CY_PIXEL_FORMAT_YCbCr422_10p_CbYCrY:                     return "YCbCr422_10p_CbYCrY";
    case CY_PIXEL_FORMAT_YCbCr422_12:                             return "YCbCr422_12";
    case CY_PIXEL_FORMAT_YCbCr422_12_CbYCrY:                      return "YCbCr422_12_CbYCrY";
    case CY_PIXEL_FORMAT_YCbCr422_12p:                            return "YCbCr422_12p";
    case CY_PIXEL_FORMAT_YCbCr422_12p_CbYCrY:                     return "YCbCr422_12p_CbYCrY";
    case CY_PIXEL_FORMAT_YCbCr601_8_CbYCr:                        return "YCbCr601_8_CbYCr";
    case CY_PIXEL_FORMAT_YCbCr601_10_CbYCr:                       return "YCbCr601_10_CbYCr";
    case CY_PIXEL_FORMAT_YCbCr601_10p_CbYCr:                      return "YCbCr601_10p_CbYCr";
    case CY_PIXEL_FORMAT_YCbCr601_12_CbYCr:                       return "YCbCr601_12_CbYCr";
    case CY_PIXEL_FORMAT_YCbCr601_12p_CbYCr:                      return "YCbCr601_12p_CbYCr";
    case CY_PIXEL_FORMAT_YCbCr601_411_8_CbYYCrYY:                 return "YCbCr601_411_8_CbYYCrYY";
    case CY_PIXEL_FORMAT_YCbCr601_422_8:                          return "YCbCr601_422_8";
    case CY_PIXEL_FORMAT_YCbCr601_422_8_CbYCrY:                   return "YCbCr601_422_8_CbYCrY";
    case CY_PIXEL_FORMAT_YCbCr601_422_10:                         return "YCbCr601_422_10";
    case CY_PIXEL_FORMAT_YCbCr601_422_10_CbYCrY:                  return "YCbCr601_422_10_CbYCrY";
    case CY_PIXEL_FORMAT_YCbCr601_422_10p:                        return "YCbCr601_422_10p";
    case CY_PIXEL_FORMAT_YCbCr601_422_10p_CbYCrY:                 return "YCbCr601_422_10p_CbYCrY";
    case CY_PIXEL_FORMAT_YCbCr601_422_12:                         return "YCbCr601_422_12";
    case CY_PIXEL_FORMAT_YCbCr601_422_12_CbYCrY:                  return "YCbCr601_422_12_CbYCrY";
    case CY_PIXEL_FORMAT_YCbCr601_422_12p:                        return "YCbCr601_422_12p";
    case CY_PIXEL_FORMAT_YCbCr601_422_12p_CbYCrY:                 return "YCbCr601_422_12p_CbYCrY";
    case CY_PIXEL_FORMAT_YCbCr709_8_CbYCr:                        return "YCbCr709_8_CbYCr";
    case CY_PIXEL_FORMAT_YCbCr709_10_CbYCr:                       return "YCbCr709_10_CbYCr";
    case CY_PIXEL_FORMAT_YCbCr709_10p_CbYCr:                      return "YCbCr709_10p_CbYCr";
    case CY_PIXEL_FORMAT_YCbCr709_12_CbYCr:                       return "YCbCr709_12_CbYCr";
    case CY_PIXEL_FORMAT_YCbCr709_12p_CbYCr:                      return "YCbCr709_12p_CbYCr";
    case CY_PIXEL_FORMAT_YCbCr709_411_8_CbYYCrYY:                 return "YCbCr709_411_8_CbYYCrYY";
    case CY_PIXEL_FORMAT_YCbCr709_422_8:                          return "YCbCr709_422_8";
    case CY_PIXEL_FORMAT_YCbCr709_422_8_CbYCrY:                   return "YCbCr709_422_8_CbYCrY";
    case CY_PIXEL_FORMAT_YCbCr709_422_10:                         return "YCbCr709_422_10";
    case CY_PIXEL_FORMAT_YCbCr709_422_10_CbYCrY:                  return "YCbCr709_422_10_CbYCrY";
    case CY_PIXEL_FORMAT_YCbCr709_422_10p:                        return "YCbCr709_422_10p";
    case CY_PIXEL_FORMAT_YCbCr709_422_10p_CbYCrY:                 return "YCbCr709_422_10p_CbYCrY";
    case CY_PIXEL_FORMAT_YCbCr709_422_12:                         return "YCbCr709_422_12";
    case CY_PIXEL_FORMAT_YCbCr709_422_12_CbYCrY:                  return "YCbCr709_422_12_CbYCrY";
    case CY_PIXEL_FORMAT_YCbCr709_422_12p:                        return "YCbCr709_422_12p";
    case CY_PIXEL_FORMAT_YCbCr709_422_12p_CbYCrY:                 return "YCbCr709_422_12p_CbYCrY";
    case CY_PIXEL_FORMAT_YCbCr2020_8_CbYCr:                       return "YCbCr2020_8_CbYCr";
    case CY_PIXEL_FORMAT_YCbCr2020_10_CbYCr:                      return "YCbCr2020_10_CbYCr";
    case CY_PIXEL_FORMAT_YCbCr2020_10p_CbYCr:                     return "YCbCr2020_10p_CbYCr";
    case CY_PIXEL_FORMAT_YCbCr2020_12_CbYCr:                      return "YCbCr2020_12_CbYCr";
    case CY_PIXEL_FORMAT_YCbCr2020_12p_CbYCr:                     return "YCbCr2020_12p_CbYCr";
    case CY_PIXEL_FORMAT_YCbCr2020_411_8_CbYYCrYY:                return "YCbCr2020_411_8_CbYYCrYY";
    case CY_PIXEL_FORMAT_YCbCr2020_422_8:                         return "YCbCr2020_422_8";
    case CY_PIXEL_FORMAT_YCbCr2020_422_8_CbYCrY:                  return "YCbCr2020_422_8_CbYCrY";
    case CY_PIXEL_FORMAT_YCbCr2020_422_10:                        return "YCbCr2020_422_10";
    case CY_PIXEL_FORMAT_YCbCr2020_422_10_CbYCrY:                 return "YCbCr2020_422_10_CbYCrY";
    case CY_PIXEL_FORMAT_YCbCr2020_422_10p:                       return "YCbCr2020_422_10p";
    case CY_PIXEL_FORMAT_YCbCr2020_422_10p_CbYCrY:                return "YCbCr2020_422_10p_CbYCrY";
    case CY_PIXEL_FORMAT_YCbCr2020_422_12:                        return "YCbCr2020_422_12";
    case CY_PIXEL_FORMAT_YCbCr2020_422_12_CbYCrY:                 return "YCbCr2020_422_12_CbYCrY";
    case CY_PIXEL_FORMAT_YCbCr2020_422_12p:                       return "YCbCr2020_422_12p";
    case CY_PIXEL_FORMAT_YCbCr2020_422_12p_CbYCrY:                return "YCbCr2020_422_12p_CbYCrY";
    case CY_PIXEL_FORMAT_YUV8_UYV:                                return "YUV8_UYV";
    case CY_PIXEL_FORMAT_YUV411_8_UYYVYY:                         return "YUV411_8_UYYVYY";
    case CY_PIXEL_FORMAT_YUV422_8:                                return "YUV422_8";
    case CY_PIXEL_FORMAT_YUV422_8_UYVY:                           return "YUV422_8_UYVY";

    case CY_PIXEL_FORMAT_GVSP_Mono10Packed:                       return "Mono10Packed";
    case CY_PIXEL_FORMAT_GVSP_Mono12Packed:                       return "Mono12Packed";
    case CY_PIXEL_FORMAT_GVSP_BayerBG10Packed:                    return "BayerBG10Packed";
    case CY_PIXEL_FORMAT_GVSP_BayerBG12Packed:                    return "BayerBG12Packed";
    case CY_PIXEL_FORMAT_GVSP_BayerGB10Packed:                    return "BayerGB10Packed";
    case CY_PIXEL_FORMAT_GVSP_BayerGB12Packed:                    return "BayerGB12Packed";
    case CY_PIXEL_FORMAT_GVSP_BayerGR10Packed:                    return "BayerGR10Packed";
    case CY_PIXEL_FORMAT_GVSP_BayerGR12Packed:                    return "BayerGR12Packed";
    case CY_PIXEL_FORMAT_GVSP_BayerRG10Packed:                    return "BayerRG10Packed";
    case CY_PIXEL_FORMAT_GVSP_BayerRG12Packed:                    return "BayerRG12Packed";
    case CY_PIXEL_FORMAT_GVSP_RGB10V1Packed:                      return "RGB10V1Packed";
    case CY_PIXEL_FORMAT_GVSP_RGB12V1Packed:                      return "RGB12V1Packed";

    case CY_PIXEL_FORMAT_Invalid:                                 return "InvalidPixelFormat";

    default: return "UnknownPixelFormat";
  }
}
#endif /* CY_PF_INCLUDE_HELPERS */

#endif /* CY_PIXEL_FORMAT_H */
