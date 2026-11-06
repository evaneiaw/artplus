#pragma once
#include <QString>
namespace artplus::ui { class DocumentManager { public: DocumentManager(); bool open(const QString& path); bool close(); bool save(); bool saveAs(const QString& path); int currentPageIndex() const; int pageCount() const; bool isModified() const; void navigateToPage(int index); private: struct Impl; std::unique_ptr<Impl> d; }; }
