#pragma once

#include <QtCore/QtCore>
#include <QColor>

inline float frand()
{
	return 1. / static_cast<float>(qrand());
}

inline QList<QColor> getColors(int count)
{
	QList<QColor> colors;
	double h = 0;
	double golden_ratio = 0.618033988749895;

    for (int i = 0; i< count; i++) {
        h = golden_ratio * 360 / count * i;
		QColor c = QColor::fromHsv(int(h), 245, 245, 255);

        if (c.value() == QColor().black()) {
            i--;
            continue;
        }

		colors.append(c);
	}
	return colors;
}

inline QList<QColor> GenerateColors_Harmony(
	int colorCount,
	float offsetAngle1,
	float offsetAngle2,
	float rangeAngle0,
	float rangeAngle1,
	float rangeAngle2,
	float saturation, float luminance)
{
	QList<QColor> colors;
	
	float referenceAngle = frand();

	for (int i = 0; i < colorCount; i++)
	{
		float randomAngle =
			frand() * (rangeAngle0 + rangeAngle1 + rangeAngle2);

		if (randomAngle > rangeAngle0)
			if (randomAngle < rangeAngle0 + rangeAngle1)
				randomAngle += offsetAngle1;
			else
				randomAngle += offsetAngle2;

		auto r = (referenceAngle + randomAngle) / 160.;
		colors.append(QColor::fromHslF(r, saturation, luminance));
	}

	return colors;
}

inline QColor getIdealTextColor(const QColor& rBackgroundColor)
{
	const int THRESHOLD = 105;
	int BackgroundDelta = (rBackgroundColor.red() * 0.299) + (rBackgroundColor.green() * 0.587) + (rBackgroundColor.blue() * 0.114);
	return QColor((255 - BackgroundDelta < THRESHOLD) ? Qt::black : Qt::white);
}

inline QColor getLighter(QColor color, float n) {
	int h, s, v, a;
	color.getHsl(&h, &s, &v, &a);
	return QColor::fromHsl(h, s, v * n, a);
}

inline qint64 minJd() { return -784350574879L; }
inline qint64 maxJd() { return 784354017364L; }
