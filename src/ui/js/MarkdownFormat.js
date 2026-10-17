.pragma library

function lineStart(text, pos) {
    const before = text.substring(0, pos)
    const idx = before.lastIndexOf("\n")
    return idx === -1 ? 0 : idx + 1
}

function lineEnd(text, pos) {
    const after = text.indexOf("\n", pos)
    return after === -1 ? text.length : after
}

function lineRange(text, pos) {
    const start = lineStart(text, pos)
    return { start: start, end: lineEnd(text, start) }
}

function replaceRange(textArea, start, end, replacement) {
    const text = textArea.text
    textArea.text = text.substring(0, start) + replacement + text.substring(end)
    const cursor = start + replacement.length
    textArea.select(cursor, cursor)
}

function stripLinePrefix(line) {
    return line.replace(/^#{1,3}\s+/, "").replace(/^-\s*(\[[ xX]\]\s*)?/, "")
}

function applyToLines(textArea, transform) {
    const text = textArea.text
    let start = textArea.selectionStart
    let end = textArea.selectionEnd

    if (start === end) {
        const range = lineRange(text, start)
        start = range.start
        end = range.end
    } else if (start > 0 && text.charAt(start - 1) !== "\n") {
        start = lineStart(text, start)
    }
    if (end < text.length && text.charAt(end - 1) !== "\n" && text.charAt(end) !== "\n") {
        end = lineEnd(text, end)
    }

    const block = text.substring(start, end)
    const lines = block.split("\n")
    const updated = lines.map(function(line) {
        return transform(stripLinePrefix(line))
    }).join("\n")

    replaceRange(textArea, start, end, updated)
}

function toggleBold(textArea) {
    const text = textArea.text
    const start = textArea.selectionStart
    const end = textArea.selectionEnd

    if (start === end) {
        replaceRange(textArea, start, end, "****")
        textArea.select(start + 2, start + 2)
        return
    }

    const selected = text.substring(start, end)
    const wrapped = "**" + selected + "**"
    replaceRange(textArea, start, end, wrapped)
    textArea.select(start, start + wrapped.length)
}

function setHeading(textArea, level) {
    const prefix = "#".repeat(level) + " "
    applyToLines(textArea, function(line) {
        return prefix + line
    })
}

function insertListItem(textArea) {
    applyToLines(textArea, function(line) {
        return "- " + line
    })
}

function insertCheckbox(textArea) {
    applyToLines(textArea, function(line) {
        return "- [ ] " + line
    })
}
