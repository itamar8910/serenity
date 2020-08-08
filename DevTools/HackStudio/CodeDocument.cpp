#include "CodeDocument.h"

namespace GUI {
NonnullRefPtr<CodeDocument> CodeDocument::create(Client* client)
{
    return adopt(*new CodeDocument(client));
}
CodeDocument::CodeDocument(Client* client)
    : TextDocument(client)
{
}
}
