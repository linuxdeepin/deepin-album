# deepin-album Expected AT-SPI Elements

## Baseline Coverage

- **Total interactive C++ widget classes**: 11
- **Existing setAccessibleName calls**: 0
- **Existing setObjectName calls**: 0 (1 commented out)
- **Pre-completion coverage**: 0%

## Expected Names

| # | AccessibleName | Role | Widget Class | File | Line (constructor) | Derivation |
|---|---|---|---|---|---|---|
| 1 | TimeLineView | panel | TimeLineView (DWidget) | timelineview.cpp | 48 | ClassName |
| 2 | ImportTimeLineView | panel | ImportTimeLineView (DWidget) | importtimelineview.cpp | - | ClassName |
| 3 | ThumbnailListView | list | ThumbnailListView (DListView) | thumbnaillistview.cpp | 88 | ClassName |
| 4 | NoResultWidget | label | NoResultWidget (QWidget) | noresultwidget.cpp | 12 | ClassName |
| 5 | FilterWidget | panel | FilterWidget (QWidget) | expansionmenu.cpp | 28 | ClassName |
| 6 | FilterLabel | label | FilterLabel (QLabel) | expansionmenu.cpp | 14 | ClassName |
| 7 | ToolButton | push button | ToolButton (DPushButton) | toolbutton.cpp | 12 | ClassName |
| 8 | ExpansionPanel | panel | ExpansionPanel (DBlurEffectWidget) | expansionpanel.cpp | 30 | ClassName |
| 9 | TimeLineDateWidget | panel | TimeLineDateWidget (DWidget) | timelinedatewidget.cpp | 17 | ClassName |
| 10 | ImportTimeLineDateWidget | panel | importTimeLineDateWidget (DWidget) | timelinedatewidget.cpp | 150 | ClassName |

## Notes

- All names follow PascalCase convention, English only, no special characters.
- The application UI is primarily QML-based. QQuickItem (QmlWidget, RubberBand,
  QImageItem, MouseEventListener, etc.) subclasses do NOT use QWidget's
  `setAccessibleName()` — they use QML `Accessible` properties instead and
  are outside the scope of this C++ AT-SPI completion pass.
- DLabel, DCheckBox, DCommandLinkButton child widgets inside parent containers
  are decorative/utility and do not need individual AT-SPI names per the
  at-spi-completion guidelines (only interactive widgets).
- Both `setObjectName()` AND `setAccessibleName()` are added for QWidget
  subclasses. QWidget subclasses only — QAction/QShortcut/QObject are excluded.