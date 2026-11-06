#pragma once

#include <QByteArray>
#include <QFile>
#include <QString>
#include <QVector>
#include <cstdio>

namespace artplus_test {

inline void appendObject(QByteArray& pdf, QVector<int>& offsets, int number,
                         const QByteArray& body)
{
    offsets[number] = pdf.size();
    pdf += QByteArray::number(number) + " 0 obj\n";
    pdf += body;
    pdf += "\nendobj\n";
}

inline QByteArray makeContentStream()
{
    QByteArray content;
    content += "0.90 0.10 0.10 rg\n";              // red rectangle fill
    content += "100 500 150 100 re f\n";
    content += "0.10 0.30 0.90 RG\n";              // blue bezier stroke
    content += "2 w\n";
    content += "350 500 m\n";
    content += "400 600 500 450 550 550 c\n";
    content += "S\n";
    content += "BT\n/F1 24 Tf\n100 400 Td\n(ArtPlus fixture) Tj\nET\n";
    content += "q\n80 0 0 80 100 200 cm\n/Im0 Do\nQ\n";     // image
    content += "q\n1 0 0 1 300 200 cm\n/Fm0 Do\nQ\n";       // form xobject
    return content;
}

// Writes a controlled fixture containing a vector rectangle, a cubic Bezier
// path, text, an image XObject and a Form XObject.
inline bool writeEditFixture(const QString& path, bool twoPages = false,
                             bool preservationResources = false, bool softMask = false)
{
    const int imageNumber = twoPages ? 9 : 8;
    const int objectCount = (twoPages ? 8 : 7) + (softMask ? 2 : 0);
    QVector<int> offsets(objectCount + 1, 0);

    QByteArray imageData;
    imageData.reserve(8 * 8 * 3);
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            imageData.append(static_cast<char>(x * 32));
            imageData.append(static_cast<char>(y * 32));
            imageData.append(static_cast<char>(128));
        }
    }

    const QByteArray formContent = "0 0.6 0 rg\n0 0 100 60 re f\n";
    QByteArray content = makeContentStream();
    if (preservationResources) {
        // Isolated bottom regions, away from the rectangle selected for editing.
        // Half-alpha red over opaque blue must render purple, not red.
        content += "q\n0 0 1 rg\n350 40 100 100 re f\n"
                   "/GSalpha gs\n1 0 0 rg\n350 40 100 100 re f\nQ\n";
        // A larger green rectangle must remain limited by the smaller clip.
        content += "q\n100 40 40 40 re W n\n0 1 0 rg\n"
                   "80 20 100 100 re f\nQ\n";
    }

    if (softMask) {
        content += "q\n0 0 1 rg\n350 40 100 100 re f\n"
                   "100 0 0 100 350 40 cm\n/Masked Do\nQ\n";
    }

    QByteArray pdf;
    pdf += "%PDF-1.7\n%\xE2\xE3\xCF\xD3\n";

    appendObject(pdf, offsets, 1, "<< /Type /Catalog /Pages 2 0 R >>");
    appendObject(pdf, offsets, 2, twoPages
                                   ? "<< /Type /Pages /Kids [3 0 R 8 0 R] /Count 2 >>"
                                   : "<< /Type /Pages /Kids [3 0 R] /Count 1 >>");
    const QByteArray resources = QByteArray(
        "/Resources << /Font << /F1 5 0 R >> "
        "/XObject << /Im0 6 0 R /Fm0 7 0 R ") +
        (softMask ? QByteArray("/Masked ") + QByteArray::number(imageNumber) + " 0 R "
                  : QByteArray()) + ">> " +
        (preservationResources
            ? "/ExtGState << /GSalpha << /Type /ExtGState /ca 0.5 /CA 0.5 >> >> "
            : "") + ">> /Contents 4 0 R >>";
    appendObject(pdf, offsets, 3,
                 "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] " + resources);

    QByteArray contentObject;
    contentObject += "<< /Length " + QByteArray::number(content.size()) + " >>\nstream\n";
    contentObject += content;
    contentObject += "endstream";
    appendObject(pdf, offsets, 4, contentObject);

    appendObject(pdf, offsets, 5,
                 "<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>");

    QByteArray imageObject;
    imageObject += "<< /Type /XObject /Subtype /Image /Width 8 /Height 8 "
                   "/ColorSpace /DeviceRGB /BitsPerComponent 8 /Length " +
                   QByteArray::number(imageData.size()) + " >>\nstream\n";
    imageObject += imageData;
    imageObject += "\nendstream";
    appendObject(pdf, offsets, 6, imageObject);

    QByteArray formObject;
    formObject += "<< /Type /XObject /Subtype /Form /BBox [0 0 100 60] "
                  "/Resources << >> /Length " +
                  QByteArray::number(formContent.size()) + " >>\nstream\n";
    formObject += formContent;
    formObject += "endstream";
    appendObject(pdf, offsets, 7, formObject);

    if (twoPages) {
        appendObject(pdf, offsets, 8,
                     "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 700 900] " + resources);
    }

    if (softMask) {
        // Red image with a grayscale /SMask: left half invisible, right half
        // half opaque. Blue backdrop distinguishes lost/ignored masks.
        QByteArray rgb, mask;
        for (int y = 0; y < 8; ++y) {
            for (int x = 0; x < 8; ++x) {
                rgb += QByteArray::fromHex("ff0000");
                mask.append(static_cast<char>(x < 4 ? 0 : 128));
            }
        }
        appendObject(pdf, offsets, imageNumber,
            "<< /Type /XObject /Subtype /Image /Width 8 /Height 8 "
            "/ColorSpace /DeviceRGB /BitsPerComponent 8 /SMask " +
            QByteArray::number(imageNumber + 1) + " 0 R /Length " +
            QByteArray::number(rgb.size()) + " >>\nstream\n" + rgb + "\nendstream");
        appendObject(pdf, offsets, imageNumber + 1,
            "<< /Type /XObject /Subtype /Image /Width 8 /Height 8 "
            "/ColorSpace /DeviceGray /BitsPerComponent 8 /Length " +
            QByteArray::number(mask.size()) + " >>\nstream\n" + mask + "\nendstream");
    }

    const int xrefOffset = pdf.size();
    pdf += "xref\n0 " + QByteArray::number(offsets.size()) + "\n";
    pdf += "0000000000 65535 f \n";
    for (int i = 1; i <= objectCount; ++i) {
        char entry[32];
        std::snprintf(entry, sizeof(entry), "%010d 00000 n \n", offsets[i]);
        pdf += entry;
    }
    pdf += "trailer\n<< /Size " + QByteArray::number(offsets.size()) +
           " /Root 1 0 R >>\nstartxref\n";
    pdf += QByteArray::number(xrefOffset);
    pdf += "\n%%EOF\n";

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    if (file.write(pdf) != pdf.size())
        return false;
    file.close();
    return true;
}

} // namespace artplus_test
