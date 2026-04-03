#include "ToolsWheelWidget.h"
#include "Rendering/DrawElements.h"
#include "Engine/Texture2D.h"
#include "Fonts/SlateFontInfo.h"

void UToolsWheelWidget::UpdateSelection(FVector2D MousePosition, FVector2D ScreenCenter)
{
	if (Slots.Num() == 0)
	{
		HoveredIndex = -1;
		return;
	}

	const FVector2D Delta = MousePosition - ScreenCenter;
	const float Distance = Delta.Size();

	if (Distance < InnerRadius)
	{
		HoveredIndex = -1;
		return;
	}

	// Atan2 gives angle from positive X axis. Offset by -90 so slot 0 is at top (12 o'clock).
	float AngleDeg = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
	AngleDeg += 90.f;
	if (AngleDeg < 0.f)
	{
		AngleDeg += 360.f;
	}

	const float SectorAngle = 360.f / Slots.Num();
	HoveredIndex = FMath::Clamp(FMath::FloorToInt(AngleDeg / SectorAngle), 0, Slots.Num() - 1);
}

int32 UToolsWheelWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const int32 NumSlots = Slots.Num();
	if (NumSlots == 0)
	{
		return LayerId;
	}

	const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
	const FVector2D Center = LocalSize * 0.5f;
	const float SectorAngleDeg = 360.f / NumSlots;
	constexpr int32 ArcSegments = 32;

	// Draw each sector
	for (int32 i = 0; i < NumSlots; ++i)
	{
		// Angles: offset by -90 so slot 0 is at top
		const float StartAngleDeg = i * SectorAngleDeg - 90.f;
		const float EndAngleDeg = StartAngleDeg + SectorAngleDeg;
		const FLinearColor& FillColor = (i == HoveredIndex) ? HighlightColor : SectorColor;

		// Build vertices for a filled annular sector (triangle strip turned into triangles)
		TArray<FSlateVertex> Vertices;
		TArray<SlateIndex> Indices;

		for (int32 Seg = 0; Seg <= ArcSegments; ++Seg)
		{
			const float Frac = static_cast<float>(Seg) / static_cast<float>(ArcSegments);
			const float AngleRad = FMath::DegreesToRadians(FMath::Lerp(StartAngleDeg, EndAngleDeg, Frac));
			const float CosA = FMath::Cos(AngleRad);
			const float SinA = FMath::Sin(AngleRad);

			// Inner vertex
			FSlateVertex InnerVert;
			InnerVert.Position = FVector2f(
				Center.X + InnerRadius * CosA,
				Center.Y + InnerRadius * SinA
			);
			InnerVert.Color = FillColor.ToFColor(true);
			InnerVert.TexCoords[0] = 0.f;
			InnerVert.TexCoords[1] = 0.f;
			InnerVert.TexCoords[2] = 0.f;
			InnerVert.TexCoords[3] = 0.f;

			// Outer vertex
			FSlateVertex OuterVert;
			OuterVert.Position = FVector2f(
				Center.X + WheelRadius * CosA,
				Center.Y + WheelRadius * SinA
			);
			OuterVert.Color = FillColor.ToFColor(true);
			OuterVert.TexCoords[0] = 0.f;
			OuterVert.TexCoords[1] = 0.f;
			OuterVert.TexCoords[2] = 0.f;
			OuterVert.TexCoords[3] = 0.f;

			Vertices.Add(InnerVert);
			Vertices.Add(OuterVert);
		}

		// Build triangle indices from the strip
		for (int32 Seg = 0; Seg < ArcSegments; ++Seg)
		{
			const SlateIndex Base = Seg * 2;
			// Triangle 1
			Indices.Add(Base);
			Indices.Add(Base + 1);
			Indices.Add(Base + 2);
			// Triangle 2
			Indices.Add(Base + 1);
			Indices.Add(Base + 3);
			Indices.Add(Base + 2);
		}

		// Transform vertices to absolute paint space
		const FSlateRenderTransform& Transform = AllottedGeometry.GetAccumulatedRenderTransform();
		for (FSlateVertex& Vert : Vertices)
		{
			const FVector2f Transformed = UE::Slate::CastToVector2f(
				Transform.TransformPoint(FVector2D(Vert.Position.X, Vert.Position.Y))
			);
			Vert.Position = Transformed;
		}

		FSlateDrawElement::MakeCustomVerts(
			OutDrawElements,
			LayerId,
			FSlateResourceHandle(),
			Vertices,
			Indices,
			nullptr,
			0,
			0
		);
	}

	// Draw separator lines between sectors
	const int32 LineLayer = LayerId + 1;
	for (int32 i = 0; i < NumSlots; ++i)
	{
		const float AngleRad = FMath::DegreesToRadians(i * SectorAngleDeg - 90.f);
		const FVector2D InnerPt = Center + FVector2D(FMath::Cos(AngleRad), FMath::Sin(AngleRad)) * InnerRadius;
		const FVector2D OuterPt = Center + FVector2D(FMath::Cos(AngleRad), FMath::Sin(AngleRad)) * WheelRadius;

		TArray<FVector2D> LinePoints;
		LinePoints.Add(InnerPt);
		LinePoints.Add(OuterPt);

		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LineLayer,
			AllottedGeometry.ToPaintGeometry(),
			LinePoints,
			ESlateDrawEffect::None,
			LineColor,
			true,
			1.5f
		);
	}

	// Draw outer highlight arc on hovered sector (GTA style)
	const int32 HighlightLayer = LayerId + 2;
	if (HoveredIndex >= 0 && HoveredIndex < NumSlots)
	{
		const float HoverStartDeg = HoveredIndex * SectorAngleDeg - 90.f;
		const float HoverEndDeg = HoverStartDeg + SectorAngleDeg;
		constexpr int32 OutlineSegments = 32;

		TArray<FVector2D> OuterArcPoints;
		for (int32 Seg = 0; Seg <= OutlineSegments; ++Seg)
		{
			const float Frac = static_cast<float>(Seg) / static_cast<float>(OutlineSegments);
			const float AngleRad = FMath::DegreesToRadians(FMath::Lerp(HoverStartDeg, HoverEndDeg, Frac));
			OuterArcPoints.Add(Center + FVector2D(FMath::Cos(AngleRad), FMath::Sin(AngleRad)) * WheelRadius);
		}

		FSlateDrawElement::MakeLines(
			OutDrawElements,
			HighlightLayer,
			AllottedGeometry.ToPaintGeometry(),
			OuterArcPoints,
			ESlateDrawEffect::None,
			HoverOutlineColor,
			true,
			HoverOutlineThickness
		);

		// Also draw the two radial edges of the hovered sector with the outline
		for (int32 Edge = 0; Edge < 2; ++Edge)
		{
			const float EdgeAngleRad = FMath::DegreesToRadians(Edge == 0 ? HoverStartDeg : HoverEndDeg);
			const FVector2D Dir(FMath::Cos(EdgeAngleRad), FMath::Sin(EdgeAngleRad));

			TArray<FVector2D> EdgePoints;
			EdgePoints.Add(Center + Dir * InnerRadius);
			EdgePoints.Add(Center + Dir * WheelRadius);

			FSlateDrawElement::MakeLines(
				OutDrawElements,
				HighlightLayer,
				AllottedGeometry.ToPaintGeometry(),
				EdgePoints,
				ESlateDrawEffect::None,
				HoverOutlineColor,
				true,
				HoverOutlineThickness
			);
		}

		// Inner arc of hovered sector
		TArray<FVector2D> InnerArcPoints;
		for (int32 Seg = 0; Seg <= OutlineSegments; ++Seg)
		{
			const float Frac = static_cast<float>(Seg) / static_cast<float>(OutlineSegments);
			const float AngleRad = FMath::DegreesToRadians(FMath::Lerp(HoverStartDeg, HoverEndDeg, Frac));
			InnerArcPoints.Add(Center + FVector2D(FMath::Cos(AngleRad), FMath::Sin(AngleRad)) * InnerRadius);
		}

		FSlateDrawElement::MakeLines(
			OutDrawElements,
			HighlightLayer,
			AllottedGeometry.ToPaintGeometry(),
			InnerArcPoints,
			ESlateDrawEffect::None,
			HoverOutlineColor,
			true,
			HoverOutlineThickness
		);
	}

	// Draw icons and text at sector midpoints
	const int32 IconLayer = LayerId + 3;
	const float MidRadius = (InnerRadius + WheelRadius) * 0.5f;
	const FSlateFontInfo FontInfo = FCoreStyle::GetDefaultFontStyle(*FontTypeFace, TextFontSize);

	for (int32 i = 0; i < NumSlots; ++i)
	{
		const float MidAngleDeg = i * SectorAngleDeg + SectorAngleDeg * 0.5f - 90.f;
		const float MidAngleRad = FMath::DegreesToRadians(MidAngleDeg);
		const FVector2D IconCenter = Center + FVector2D(FMath::Cos(MidAngleRad), FMath::Sin(MidAngleRad)) * MidRadius;

		// Draw icon if available
		if (Slots[i].Icon != nullptr)
		{
			const FVector2D IconPos = IconCenter - FVector2D(IconSize * 0.5f, IconSize * 0.5f + 10.f);
			FSlateBrush IconBrush;
			IconBrush.SetResourceObject(Slots[i].Icon);
			IconBrush.ImageSize = FVector2D(IconSize, IconSize);

			FSlateDrawElement::MakeBox(
				OutDrawElements,
				IconLayer,
				AllottedGeometry.ToPaintGeometry(FVector2D(IconSize, IconSize), FSlateLayoutTransform(IconPos)),
				&IconBrush,
				ESlateDrawEffect::None,
				FLinearColor::White
			);
		}

		// Draw display name below icon
		const FText& Name = Slots[i].DisplayName;
		if (!Name.IsEmpty())
		{
			const FVector2D TextPos = IconCenter + FVector2D(-40.f, IconSize * 0.3f);
			FSlateDrawElement::MakeText(
				OutDrawElements,
				IconLayer,
				AllottedGeometry.ToPaintGeometry(FVector2D(80.f, 20.f), FSlateLayoutTransform(TextPos)),
				Name.ToString(),
				FontInfo,
				ESlateDrawEffect::None,
				TextColor
			);
		}
	}

	return IconLayer;
}
