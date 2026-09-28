import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Dialogs as Dialogs
import QtQuick.Window

ApplicationWindow {
    id: win
    width: 1100
    height: 760
    minimumWidth: 680
    minimumHeight: 480
    visible: true
    title: reader.hasBook ? reader.title + " — Omabook" : "Omabook"
    color: omarchyTheme.background
    Material.theme: omarchyTheme.dark ? Material.Dark : Material.Light
    Material.accent: omarchyTheme.accent

    readonly property real textScale: omarchyTheme.textScale
    readonly property color foreground: omarchyTheme.foreground
    readonly property color muted: omarchyTheme.muted
    readonly property int bodySize: Math.round(reader.fontSize * textScale)
    property bool contentsOpen: false
    property bool searchOpen: false
    property var searchResults: []
    property int searchResultIndex: 0
    property int viewOffset: 0
    property url pendingExternalUrl
    function px(value) { return Math.round(value * textScale) }
    function openSearch() {
        if (!reader.hasBook) return
        contentsOpen = false
        searchOpen = true
        searchField.forceActiveFocus()
    }
    function openContents() {
        if (!reader.hasBook) return
        searchOpen = false
        contentsOpen = !contentsOpen
    }
    function openSelectedSearchResult() {
        if (searchResults.length < 1) return
        const result = searchResults[Math.max(0, Math.min(searchResultIndex, searchResults.length - 1))]
        reader.openSearchResult(result.chapter, result.offset)
        searchOpen = false
    }
    function moveSearch(delta) {
        if (searchResults.length < 1) return
        searchResultIndex = (searchResultIndex + delta + searchResults.length) % searchResults.length
    }
    function restoreViewport(offset) {
        viewOffset = offset
        Qt.callLater(function() {
            if (!reader.hasBook) return
            const rect = chapterText.positionToRectangle(Math.min(offset, chapterText.length))
            readingFlick.contentY = Math.max(0, Math.min(readingFlick.contentHeight - readingFlick.height,
                                                          chapterText.y + rect.y))
        })
    }
    onClosing: if (reader.hasBook) reader.savePosition(chapterText.positionAt(4,
        Math.max(0, readingFlick.contentY - chapterText.y) + px(12)))
    onWidthChanged: resizeRestore.restart()
    onBodySizeChanged: resizeRestore.restart()

    Timer { id: resizeRestore; interval: 120; onTriggered: if (reader.hasBook) win.restoreViewport(win.viewOffset) }
    Shortcut { sequence: "Ctrl+O"; onActivated: openDialog.open() }
    Shortcut { sequence: "Ctrl+T"; onActivated: win.openContents() }
    Shortcut { sequence: "Ctrl+F"; onActivated: win.openSearch() }
    Shortcut { sequence: "Shift+Enter"; enabled: win.searchOpen; onActivated: win.moveSearch(-1) }
    Shortcut { sequence: "Ctrl++"; onActivated: reader.setFontSize(reader.fontSize + 1) }
    Shortcut { sequence: "Ctrl+-"; onActivated: reader.setFontSize(reader.fontSize - 1) }
    Shortcut { sequence: "Alt+Right"; onActivated: reader.nextChapter() }
    Shortcut { sequence: "Alt+Left"; onActivated: reader.previousChapter() }
    Shortcut { sequence: "F11"; onActivated: win.visibility = win.visibility === Window.FullScreen ? Window.Windowed : Window.FullScreen }
    Shortcut {
        sequence: "Escape"
        onActivated: {
            if (searchOpen) searchOpen = false
            else if (contentsOpen) contentsOpen = false
            else if (win.visibility === Window.FullScreen) win.visibility = Window.Windowed
        }
    }
    Shortcut { sequence: "PageDown"; onActivated: readingFlick.contentY = Math.min(readingFlick.contentHeight - readingFlick.height, readingFlick.contentY + readingFlick.height * 0.85) }
    Shortcut { sequence: "PageUp"; onActivated: readingFlick.contentY = Math.max(0, readingFlick.contentY - readingFlick.height * 0.85) }
    Shortcut { sequence: "Space"; enabled: !win.searchOpen; onActivated: readingFlick.contentY = Math.min(readingFlick.contentHeight - readingFlick.height, readingFlick.contentY + readingFlick.height * 0.85) }
    Shortcut { sequence: "Shift+Space"; enabled: !win.searchOpen; onActivated: readingFlick.contentY = Math.max(0, readingFlick.contentY - readingFlick.height * 0.85) }

    Dialogs.FileDialog {
        id: openDialog
        title: "Open EPUB"
        fileMode: Dialogs.FileDialog.OpenFile
        nameFilters: ["EPUB books (*.epub)"]
        onAccepted: {
            win.searchOpen = false
            win.contentsOpen = false
            reader.open(selectedFile)
        }
    }
    Connections {
        target: reader
        function onRestorePosition(offset) { win.restoreViewport(offset) }
        function onExternalLinkRequested(url) { win.pendingExternalUrl = url; externalDialog.open() }
    }
    Dialog {
        id: externalDialog
        title: "Open external link?"
        modal: true
        anchors.centerIn: parent
        width: Math.min(win.width - win.px(40), win.px(460))
        standardButtons: Dialog.Yes | Dialog.No
        contentItem: Label {
            text: win.pendingExternalUrl.toString()
            color: win.foreground
            wrapMode: Text.WrapAnywhere
            width: externalDialog.width - win.px(40)
        }
        onAccepted: Qt.openUrlExternally(win.pendingExternalUrl)
    }

    Item {
        anchors.fill: parent
        Rectangle { anchors.fill: parent; color: omarchyTheme.background }

        Label {
            visible: !reader.hasBook && !reader.loading
            x: Math.max(win.px(24), (parent.width - width) / 2)
            y: win.px(58)
            width: Math.min(win.px(680), parent.width - win.px(48))
            text: "Open an EPUB to read."
            color: win.muted
            font.family: "serif"
            font.pixelSize: win.px(20)
        }
        Label {
            visible: reader.loading
            anchors.centerIn: parent
            text: "Opening…"
            color: win.muted
            font.pixelSize: win.px(14)
        }
        Flickable {
            id: readingFlick
            anchors.fill: parent
            anchors.bottomMargin: win.px(32)
            visible: reader.hasBook
            clip: true
            contentWidth: width
            contentHeight: Math.max(height, chapterText.implicitHeight + win.px(130))
            boundsBehavior: Flickable.StopAtBounds
            onMovementEnded: positionTimer.restart()
            onContentYChanged: if (visible && !moving) positionTimer.restart()
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded; bottomPadding: win.px(28) }
            Timer {
                id: positionTimer
                interval: 600
                onTriggered: if (reader.hasBook) {
                    win.viewOffset = chapterText.positionAt(4, Math.max(0, readingFlick.contentY - chapterText.y) + win.px(12))
                    reader.savePosition(win.viewOffset)
                }
            }
            TextEdit {
                id: chapterText
                x: Math.max(win.px(24), (readingFlick.width - width) / 2)
                y: win.px(58)
                width: Math.min(win.px(680), readingFlick.width - win.px(48))
                height: implicitHeight
                readOnly: true
                selectByMouse: true
                wrapMode: TextEdit.Wrap
                textFormat: TextEdit.RichText
                text: reader.chapterHtml
                color: win.foreground
                selectionColor: omarchyTheme.selection
                font.family: "serif"
                font.pixelSize: win.bodySize
                onLinkActivated: function(link) { reader.openLink(link) }
            }
        }

        Rectangle {
            id: contentsPanel
            visible: win.contentsOpen && reader.hasBook
            anchors.left: parent.left
            anchors.bottom: footer.top
            anchors.leftMargin: win.px(12)
            anchors.bottomMargin: win.px(12)
            width: Math.min(win.px(310), parent.width - win.px(24))
            height: Math.min(win.px(430), parent.height - win.px(80))
            color: omarchyTheme.background
            border.color: win.muted
            border.width: 1
            ListView {
                anchors.fill: parent
                anchors.margins: win.px(8)
                clip: true
                model: reader.contents
                delegate: ItemDelegate {
                    required property int index
                    required property string modelData
                    width: ListView.view.width
                    text: modelData
                    highlighted: index === reader.chapterIndex
                    onClicked: { reader.goToChapter(index); win.contentsOpen = false }
                }
            }
        }
        Rectangle {
            id: searchPanel
            visible: win.searchOpen && reader.hasBook
            anchors.left: parent.left
            anchors.bottom: footer.top
            anchors.leftMargin: win.px(12)
            anchors.bottomMargin: win.px(12)
            width: Math.min(win.px(440), parent.width - win.px(24))
            height: Math.min(win.px(350), parent.height - win.px(80))
            color: omarchyTheme.background
            border.color: win.muted
            border.width: 1
            Column {
                anchors.fill: parent
                anchors.margins: win.px(8)
                spacing: win.px(6)
                TextField {
                    id: searchField
                    width: parent.width
                    placeholderText: "Search this book"
                    onTextChanged: { win.searchResults = reader.search(text); win.searchResultIndex = 0 }
                    onAccepted: win.openSelectedSearchResult()
                }
                Label {
                    text: searchField.text.length < 2 ? "Type at least two letters" :
                        (win.searchResults.length >= 200 ? "200+" : win.searchResults.length) + " matches"
                    color: win.muted
                    font.pixelSize: win.px(11)
                }
                ListView {
                    width: parent.width
                    height: parent.height - searchField.height - win.px(28)
                    clip: true
                    model: win.searchResults
                    delegate: ItemDelegate {
                        required property var modelData
                        required property int index
                        width: ListView.view.width
                        height: win.px(66)
                        highlighted: index === win.searchResultIndex
                        text: modelData.title + "\n" + modelData.excerpt
                        onClicked: { win.searchResultIndex = index; win.openSelectedSearchResult() }
                    }
                }
            }
        }

        Item {
            id: footer
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: win.px(34)
            Row {
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                anchors.leftMargin: win.px(12)
                anchors.bottomMargin: win.px(10)
                spacing: win.px(12)
                opacity: 0.65
                FooterIconButton { iconName: "folder"; iconColor: win.muted; uiScale: win.textScale; tooltip: "Open EPUB (Ctrl+O)"; onClicked: openDialog.open() }
                FooterIconButton { iconName: "contents"; iconColor: win.muted; uiScale: win.textScale; enabled: reader.hasBook; tooltip: "Contents (Ctrl+T)"; onClicked: win.openContents() }
                FooterIconButton { iconName: "search"; iconColor: win.muted; uiScale: win.textScale; enabled: reader.hasBook; tooltip: "Search (Ctrl+F)"; onClicked: win.openSearch() }
                FooterIconButton { iconName: "minus"; iconColor: win.muted; uiScale: win.textScale; enabled: reader.hasBook; tooltip: "Smaller text (Ctrl+-)"; onClicked: reader.setFontSize(reader.fontSize - 1) }
                FooterIconButton { iconName: "plus"; iconColor: win.muted; uiScale: win.textScale; enabled: reader.hasBook; tooltip: "Larger text (Ctrl++)"; onClicked: reader.setFontSize(reader.fontSize + 1) }
                FooterIconButton { iconName: "fullscreen"; iconColor: win.muted; uiScale: win.textScale; tooltip: "Full screen (F11)"; onClicked: win.visibility = win.visibility === Window.FullScreen ? Window.Windowed : Window.FullScreen }
                Label {
                    visible: reader.hasBook
                    text: reader.chapterTitle
                    color: win.muted
                    font.pixelSize: win.px(11)
                    width: Math.min(win.px(280), Math.max(0, win.width / 3))
                    height: win.px(16)
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                }
            }
            Label {
                visible: reader.hasBook
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.rightMargin: win.px(12)
                anchors.bottomMargin: win.px(10)
                text: reader.progress + "%"
                color: win.muted
                opacity: 0.75
                font.pixelSize: win.px(11)
            }
        }
        Rectangle {
            visible: reader.error.length > 0
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: footer.top
            anchors.bottomMargin: win.px(12)
            width: Math.min(parent.width - win.px(32), win.px(550))
            height: errorLabel.implicitHeight + win.px(22)
            radius: win.px(5)
            color: omarchyTheme.dark ? "#333333" : "#e6dcc9"
            Label {
                id: errorLabel
                anchors.centerIn: parent
                width: parent.width - win.px(20)
                text: reader.error
                color: win.foreground
                wrapMode: Text.Wrap
                font.pixelSize: win.px(12)
            }
        }
    }
}
