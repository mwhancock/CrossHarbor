#include "BootActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>

#include "AppVersion.h"
#include "fontIds.h"
#include "images/Logo120.h"

namespace {
void drawCrossHarborLogo(GfxRenderer& renderer, int centerX, int centerY) {
  constexpr int logoHalf = 60;
  renderer.drawImage(Logo120, centerX - logoHalf, centerY - logoHalf, 120, 120);

  // Carve an anchor shape out of the existing droplet mark.
  renderer.fillRect(centerX - 5, centerY - 38, 10, 58, false);   // shaft
  renderer.fillRect(centerX - 20, centerY - 8, 40, 8, false);    // crossbar
  renderer.fillRect(centerX - 8, centerY - 50, 16, 10, false);   // ring opening
  renderer.fillRect(centerX - 18, centerY + 18, 36, 8, false);   // fluke bridge

  const int leftX[3] = {centerX - 3, centerX - 30, centerX - 14};
  const int leftY[3] = {centerY + 22, centerY + 42, centerY + 42};
  const int rightX[3] = {centerX + 3, centerX + 30, centerX + 14};
  const int rightY[3] = {centerY + 22, centerY + 42, centerY + 42};
  renderer.fillPolygon(leftX, leftY, 3, false);
  renderer.fillPolygon(rightX, rightY, 3, false);

  // Re-outline key anchor edges so the mark stays readable at boot distance.
  renderer.drawRect(centerX - 10, centerY - 52, 20, 14, true);
  renderer.drawLine(centerX, centerY - 38, centerX, centerY + 24, true);
  renderer.drawLine(centerX - 20, centerY - 4, centerX + 20, centerY - 4, true);
  renderer.drawLine(centerX - 3, centerY + 22, centerX - 28, centerY + 40, true);
  renderer.drawLine(centerX + 3, centerY + 22, centerX + 28, centerY + 40, true);
}
}  // namespace

void BootActivity::onEnter() {
  Activity::onEnter();

  const auto pageWidth = renderer.getScreenWidth();
  const auto pageHeight = renderer.getScreenHeight();

  renderer.clearScreen();
  drawCrossHarborLogo(renderer, pageWidth / 2, pageHeight / 2);
  renderer.drawCenteredText(UI_10_FONT_ID, pageHeight / 2 + 70, tr(STR_CROSSHARBOR), true, EpdFontFamily::BOLD);
  renderer.drawCenteredText(SMALL_FONT_ID, pageHeight / 2 + 95, tr(STR_BOOTING));
  renderer.drawCenteredText(SMALL_FONT_ID, pageHeight - 30, CROSSINK_VERSION);
  renderer.displayBuffer();
}
