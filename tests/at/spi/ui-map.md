# deepin-album UI Map (AT-SPI)

## Architecture Overview

deepin-album is a **QML-first** application with embedded C++ widgets. The main
window is defined in QML (`qrc:/qml/main.qml`). C++ DWidget/QWidget subclasses
are embedded inside `QmlWidget` (a `QQuickPaintedItem`) and rendered via
`render()` in `paint()`. These C++ widgets implement the thumbnail grid, timeline
headers, filter dropdown, and empty-state views.

## Component Tree (C++ Widgets Only)

```
DApplication
 └─ QQmlApplicationEngine (QML main.qml)
     └─ QmlWidget (QQuickPaintedItem)
         ├─ TimeLineView (DWidget) [day view]
         │   ├─ DWidget (m_timeLineViewWidget) [container]
         │   │   ├─ ThumbnailListView (DListView) [thumbnail grid]
         │   │   │   └─ TimeLineDateWidget (DWidget) [timeline section header] *
         │   │   │   └─ importTimeLineDateWidget (DWidget) [import section header] *
         │   │   └─ NoResultWidget (QWidget) [empty state]
         │   └─ DWidget (m_dateNumItemWidget) [floating title bar]
         │       ├─ DLabel (m_dateLabel) [date text]
         │       ├─ DCheckBox (m_numCheckBox) [select all checkbox]
         │       ├─ DLabel (m_numLabel) [count text]
         │       └─ FilterWidget (m_ToolButton) [filter dropdown button]
         │           └─ FilterLabel(s) [filter options]
         │               └─ ToolButton(s) (DPushButton) [filter items in ExpansionPanel]
         │
         └─ ImportTimeLineView (DWidget) [imported view]
             ├─ DWidget (m_timeLineViewWidget) [container]
             │   ├─ ThumbnailListView (m_importTimeLineListView) [thumbnail grid]
             │   └─ NoResultWidget (m_noResultWidget) [empty state]
             └─ DWidget (m_importTitleItem) [floating title bar]
                 ├─ DLabel (m_importLabel) [import date label]
                 ├─ DLabel (m_dateNumLabel) [date+count label]
                 ├─ DCheckBox (m_dateNumCheckBox) [select all checkbox]
                 └─ FilterWidget (m_ToolButton) [filter dropdown button]
```

> * `TimeLineDateWidget` / `importTimeLineDateWidget` are created dynamically
>   per timeline section via the ThumbnailDelegate (painted as list items).

## Interactive Controls

| Widget | Class | Type | Accessible Path |
|--------|-------|------|-----------------|
| Day view container | TimeLineView | DWidget | timeline/day |
| Imported view container | ImportTimeLineView | DWidget | timeline/imported |
| Thumbnail grid (day) | ThumbnailListView | DListView | thumbnail/grid-day |
| Thumbnail grid (imported) | ThumbnailListView | DListView | thumbnail/grid-imported |
| Empty state | NoResultWidget | QWidget | generic/no-result |
| Floating date label | DLabel | DLabel | timeline/date-label |
| Select-all checkbox | DCheckBox | DCheckBox | timeline/select-all |
| Count label | DLabel | DLabel | timeline/count-label |
| Filter button | FilterWidget | QWidget | timeline/filter-button |
| Filter panel | ExpansionPanel | DBlurEffectWidget | timeline/filter-panel |
| Filter "All" button | ToolButton | DPushButton | timeline/filter-all |
| Filter "Photos" button | ToolButton | DPushButton | timeline/filter-photos |
| Filter "Videos" button | ToolButton | DPushButton | timeline/filter-videos |
| Filter label (each) | FilterLabel | QLabel | timeline/filter-label |
| Timeline section header | TimeLineDateWidget | DWidget | timeline/section-header |
| Import section header | importTimeLineDateWidget | DWidget | timeline/import-section-header |

## File Index

| File | Lines | Content |
|------|-------|---------|
| src/src/widgets/timelineview/timelineview.cpp | 1- | TimeLineView implementation |
| src/src/widgets/importtimelineview/importtimelineview.cpp | 1- | ImportTimeLineView implementation |
| src/src/widgets/thumbnail/thumbnaillistview.cpp | 1- | ThumbnailListView implementation |
| src/src/widgets/widgtes/expansionmenu.cpp | 1- | FilterWidget, FilterLabel, ExpansionMenu |
| src/src/widgets/widgtes/expansionpanel.cpp | 1- | ExpansionPanel, ToolButton creation |
| src/src/widgets/widgtes/toolbutton.cpp | 1- | ToolButton implementation |
| src/src/widgets/widgtes/noresultwidget.cpp | 1- | NoResultWidget implementation |
| src/src/widgets/thumbnail/timelinedatewidget.cpp | 1- | TimeLineDateWidget, importTimeLineDateWidget |