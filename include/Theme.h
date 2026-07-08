#pragma once

#include <Arduino.h>

// Shared palette + base styles for every device-hosted page.
// static: this header is included by multiple .cpp files, each needs its own copy.
static const char THEME_CSS[] PROGMEM = R"CSS(
:root {
  --navy: #0B2D48;
  --water: #1A6FA8;
  --bg: #EEF5FB;
  --white: #FFFFFF;
  --pale: #C8DFF0;
  --inset: #F0F7FC;
  --text: #1A2B3C;
  --muted: #5E7D96;
}
* { box-sizing: border-box; }
body {
  margin: 0;
  font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
  background: var(--bg);
  color: var(--text);
}
.box {
  background: var(--white);
  border: 2px solid var(--pale);
  border-radius: 12px;
  padding: 16px;
}
.box-title {
  color: var(--muted);
  text-transform: uppercase;
  font-size: 12px;
  font-weight: 700;
  letter-spacing: 0.04em;
  padding-bottom: 8px;
  margin-bottom: 12px;
  border-bottom: 1px solid var(--pale);
}
.row {
  display: flex;
  justify-content: space-between;
  gap: 12px;
  padding: 6px 0;
  font-size: 14px;
}
.row + .row { border-top: 1px solid var(--inset); }
.metric-card {
  background: var(--white);
  border: 2px solid var(--water);
  border-radius: 12px;
  padding: 16px;
}
.metric-value {
  color: var(--navy);
  font-size: 32px;
  font-weight: 700;
}
.page {
  max-width: 540px;
  margin: 0 auto;
  padding: 20px;
  display: flex;
  flex-direction: column;
  gap: 16px;
}
.page.wide { max-width: 900px; }
)CSS";
