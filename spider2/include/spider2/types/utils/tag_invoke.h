#pragma once

namespace spider2
{
   /**
    * @brief A tag type to be used in tag_invoke overloads.
    * It takes reference to CPO point and returns the type of the point.
    * @tparam Tag CPO object
    */
   template <auto &Tag>
   using tag_t = std::decay_t<decltype(Tag)>;
} // namespace spider2